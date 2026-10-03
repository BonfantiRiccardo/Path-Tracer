#include "path_tracer_impl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace vkgpu {

/**
 * Dispatches the compute shader to perform path tracing.
 *
 * The full sample count is split across several dispatches ("sample batches") rather than being traced in a
 * single dispatch. A single dispatch that traces every sample and bounce for a heavy render can run longer
 * than the operating system's GPU watchdog timeout (Timeout Detection and Recovery, ~2 s on Windows); when
 * that happens the driver resets the device mid-dispatch, the not-yet-executed workgroups never write their
 * pixels, and the image ends up with a black region (typically the lower rows). Bounding the per-dispatch
 * work keeps every submission comfortably under that limit. Each batch accumulates into the output buffer,
 * which is zeroed up front, and the host normalizes by the accumulated sample count when tonemapping.
 */
void PathTracer::Impl::dispatch() {
    zeroOutputBuffer();

    // Cap the ray-bounce work per pixel per dispatch. Fewer bounces allow more samples per batch and
    // vice versa, so the work per pixel in each submission stays bounded whatever --spp and --bounces are
    // (it still grows with resolution and sphere count).
    constexpr uint32_t kRayBudgetPerBatch = 512u;
    const uint32_t samplesPerBatch = std::clamp(
        kRayBudgetPerBatch / std::max(1u, config_.maxBounces),
        1u,
        std::max(1u, config_.samplesPerPixel)
    );

    // Define the local workgroup size and calculate the number of groups needed to cover the entire image.
    constexpr uint32_t localSizeX = 16;
    constexpr uint32_t localSizeY = 16;
    const uint32_t groupsX = (config_.width + localSizeX - 1) / localSizeX;
    const uint32_t groupsY = (config_.height + localSizeY - 1) / localSizeY;

    for (uint32_t sampleOffset = 0; sampleOffset < config_.samplesPerPixel; sampleOffset += samplesPerBatch) {
        const uint32_t batchSamples = std::min(samplesPerBatch, config_.samplesPerPixel - sampleOffset);
        dispatchSampleBatch(groupsX, groupsY, sampleOffset, batchSamples);
    }
}

/**
 * Clears the output accumulation buffer to zero before rendering begins.
 *
 * The buffer is HOST_VISIBLE | HOST_COHERENT, and host writes issued before a queue submission are
 * guaranteed to be visible to that submission, so a plain host-side memset is sufficient and no explicit
 * host-write barrier is required before the first dispatch reads it.
 */
void PathTracer::Impl::zeroOutputBuffer() {
    const VkDeviceSize outputBufferSize =
        static_cast<VkDeviceSize>(config_.width) * static_cast<VkDeviceSize>(config_.height) * sizeof(OutputPixel);

    void* mapped = nullptr;
    if (vkMapMemory(device_, outputBufferMemory_, 0, outputBufferSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map output buffer for clearing.");
    }
    std::memset(mapped, 0, static_cast<size_t>(outputBufferSize));
    vkUnmapMemory(device_, outputBufferMemory_);
}

/**
 * Records, submits, and waits for a single sample batch. Each batch traces batchSamples samples per pixel,
 * offset by sampleOffset (which also decorrelates the batch's random sequence), and adds the result into the
 * output accumulation buffer.
 */
void PathTracer::Impl::dispatchSampleBatch(uint32_t groupsX, uint32_t groupsY, uint32_t sampleOffset, uint32_t batchSamples) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Begin recording commands into the command buffer (implicitly resets it; the pool was created with
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT so it can be re-recorded for each batch).
    if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer.");
    }

    // Ensure the previous batch's accumulating writes to the output buffer are visible to this batch's
    // read-modify-write. Harmless on the first batch (the host zero-fill is already visible via submission).
    VkBufferMemoryBarrier accumulateBarrier{};
    accumulateBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    accumulateBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    accumulateBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    accumulateBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumulateBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumulateBarrier.buffer = outputBuffer_;
    accumulateBarrier.offset = 0;
    accumulateBarrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0,
        nullptr,
        1,
        &accumulateBarrier,
        0,
        nullptr
    );

    // Bind the compute pipeline and descriptor sets
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout_,
        0,
        1,
        &descriptorSet_,
        0,
        nullptr
    );

    // Push constants to the shader. samplesPerPixel carries this batch's sample count, not the total.
    PushConstants push{};
    push.width = config_.width;
    push.height = config_.height;
    push.sphereCount = static_cast<uint32_t>(sceneData_.size());
    push.samplesPerPixel = batchSamples;
    push.maxBounces = config_.maxBounces;
    push.seed = config_.seed;
    push.sampleOffset = sampleOffset;

    vkCmdPushConstants(commandBuffer_, pipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &push);

    vkCmdDispatch(commandBuffer_, groupsX, groupsY, 1);

    // Insert a memory barrier to ensure that the compute shader has finished writing to the output buffer before we read it on the host
    VkBufferMemoryBarrier outputBarrier{};
    outputBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    outputBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    outputBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    outputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    outputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    outputBarrier.buffer = outputBuffer_;
    outputBarrier.offset = 0;
    outputBarrier.size = VK_WHOLE_SIZE;

    // Ensure that the compute shader writes are visible to the host before we read the output buffer
    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        0,
        nullptr,
        1,
        &outputBarrier,
        0,
        nullptr
    );

    if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer.");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(device_, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence.");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    if (vkQueueSubmit(computeQueue_, 1, &submitInfo, fence) != VK_SUCCESS) {
        vkDestroyFence(device_, fence, nullptr);
        throw std::runtime_error("Failed to submit compute command buffer.");
    }

    const VkResult waitResult = vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device_, fence, nullptr);

    // A device loss here usually means a dispatch exceeded the GPU watchdog timeout. Surface it instead of
    // silently writing a partially rendered (black) image and reporting success.
    if (waitResult == VK_ERROR_DEVICE_LOST) {
        throw std::runtime_error(
            "GPU device lost while rendering (likely a compute dispatch exceeded the OS GPU watchdog timeout). "
            "Try lowering --spp or --bounces."
        );
    }
    if (waitResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for compute fence.");
    }
}

/**
 * Helper function to tonemap a linear color value to an 8-bit byte.
 */
uint8_t PathTracer::Impl::tonemapToByte(float linear) {
    const float gamma2 = std::sqrt(std::clamp(linear, 0.0f, 0.999f));
    const float scaled = std::clamp(gamma2 * 256.0f, 0.0f, 255.0f);
    return static_cast<uint8_t>(scaled);
}

/**
 * Writes the rendered image to a PPM file.
 */
void PathTracer::Impl::writePpmImage() {
    const VkDeviceSize outputBufferSize = static_cast<VkDeviceSize>(config_.width) * static_cast<VkDeviceSize>(config_.height) * sizeof(OutputPixel);

    void* mapped = nullptr;
    if (vkMapMemory(device_, outputBufferMemory_, 0, outputBufferSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map output buffer.");
    }

    const auto* pixels = reinterpret_cast<const OutputPixel*>(mapped);

    if (!config_.outputPath.parent_path().empty()) {
        std::filesystem::create_directories(config_.outputPath.parent_path());
    }

    std::ofstream outFile(config_.outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        vkUnmapMemory(device_, outputBufferMemory_);
        throw std::runtime_error("Failed to open output file: " + config_.outputPath.string());
    }

    outFile << "P6\n" << config_.width << " " << config_.height << "\n255\n";

    for (uint32_t y = 0; y < config_.height; ++y) {
        for (uint32_t x = 0; x < config_.width; ++x) {
            const OutputPixel& pixel = pixels[y * config_.width + x];
            // The shader accumulates the summed radiance across all sample batches and stores the total
            // sample count in the alpha channel; normalize by it to recover the averaged color.
            const float invSamples = pixel.a > 0.0f ? 1.0f / pixel.a : 0.0f;
            const uint8_t rgb[3] = {
                tonemapToByte(pixel.r * invSamples),
                tonemapToByte(pixel.g * invSamples),
                tonemapToByte(pixel.b * invSamples)
            };
            outFile.write(reinterpret_cast<const char*>(rgb), 3);
        }
    }

    outFile.close();
    vkUnmapMemory(device_, outputBufferMemory_);
}

/**
 * Cleans up all Vulkan resources. This function is called in the destructor to ensure that all resources are properly released.
 */
void PathTracer::Impl::cleanup() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }

    if (outputBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, outputBuffer_, nullptr);
        outputBuffer_ = VK_NULL_HANDLE;
    }

    if (outputBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, outputBufferMemory_, nullptr);
        outputBufferMemory_ = VK_NULL_HANDLE;
    }

    if (sceneBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, sceneBuffer_, nullptr);
        sceneBuffer_ = VK_NULL_HANDLE;
    }

    if (sceneBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, sceneBufferMemory_, nullptr);
        sceneBufferMemory_ = VK_NULL_HANDLE;
    }

    if (cameraBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, cameraBuffer_, nullptr);
        cameraBuffer_ = VK_NULL_HANDLE;
    }

    if (cameraBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, cameraBufferMemory_, nullptr);
        cameraBufferMemory_ = VK_NULL_HANDLE;
    }

    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }

    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

}  // namespace vkgpu
