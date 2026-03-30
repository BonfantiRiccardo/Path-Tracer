#ifndef VULKAN_GPU_RT_PATH_TRACER_IMPL_H
#define VULKAN_GPU_RT_PATH_TRACER_IMPL_H

#include <vulkan/vulkan.h>

#include "path_tracer_app.h"
#include "scene.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace vkgpu {

/**
 * Internal implementation class for the PathTracer application. 
 * Encapsulates all Vulkan resources, scene data, and rendering logic.
 */
class PathTracer::Impl {
public:
    explicit Impl(RenderConfig config);
    ~Impl();

    void run();

private:
    static std::optional<uint32_t> findComputeQueueFamily(VkPhysicalDevice device);
    static std::vector<GpuSphere> uploadableScene(const std::vector<SceneSphere>& scene);
    static GpuCamera uploadableCamera(const SceneCamera& camera);
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory
    );
    void createBuffers();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();
    std::filesystem::path resolveShaderPath() const;
    void createComputePipeline();
    void updateDescriptorSet();

    void dispatch();
    static uint8_t tonemapToByte(float linear);
    void writePpmImage();
    void cleanup();

private:
    RenderConfig config_{};

    // Initialize Vulkan handles to null
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties_{};
    VkDevice device_ = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex_ = 0;
    VkQueue computeQueue_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkBuffer sceneBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory sceneBufferMemory_ = VK_NULL_HANDLE;

    VkBuffer outputBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory outputBufferMemory_ = VK_NULL_HANDLE;

    VkBuffer cameraBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory cameraBufferMemory_ = VK_NULL_HANDLE;

    SceneDescription sceneDescription_{};
    std::vector<GpuSphere> sceneData_;
    GpuCamera cameraData_{};
};

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_PATH_TRACER_IMPL_H