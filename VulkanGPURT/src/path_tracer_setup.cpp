#include "path_tracer_impl.h"

#include "shader_file_io.h"

#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace vkgpu {

/**
 * Finds a queue family index that supports compute operations. A queue family is a group of queues that have the same capabilities.
 * This function queries the physical device for its queue families and checks if any of them support the VK_QUEUE_COMPUTE_BIT flag,
 * which indicates that they can be used for compute operations. If a suitable queue family is found, its index is returned.
 */
std::optional<uint32_t> PathTracer::Impl::findComputeQueueFamily(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
            return i;
        }
    }

    return std::nullopt;
}

/**
 * Converts a vector of scene spheres to a vector of GPU spheres.
 */
std::vector<GpuSphere> PathTracer::Impl::uploadableScene(const std::vector<SceneSphere>& scene) {
    std::vector<GpuSphere> out;
    out.reserve(scene.size());

    for (const SceneSphere& sphere : scene) {
        GpuSphere gpu{};
        gpu.centerRadius = {sphere.center[0], sphere.center[1], sphere.center[2], sphere.radius};
        gpu.albedoMaterialType = {
            sphere.material.albedo[0],
            sphere.material.albedo[1],
            sphere.material.albedo[2],
            static_cast<float>(static_cast<uint32_t>(sphere.material.type))
        };
        gpu.materialParams = {sphere.material.fuzz, sphere.material.refractionIndex, 0.0f, 0.0f};
        out.push_back(gpu);
    }

    return out;
}

/**
 * Converts a scene camera to a GPU camera.
 */
GpuCamera PathTracer::Impl::uploadableCamera(const SceneCamera& camera) {
    GpuCamera gpu{};
    gpu.lookFromDefocusAngle = {camera.lookfrom[0], camera.lookfrom[1], camera.lookfrom[2], camera.defocusAngle};
    gpu.lookAtFocusDist = {camera.lookat[0], camera.lookat[1], camera.lookat[2], camera.focusDist};
    gpu.viewUpVfov = {camera.viewup[0], camera.viewup[1], camera.viewup[2], camera.vfov};
    return gpu;
}

/**
 * Finds a suitable memory type for a Vulkan buffer. This function queries the physical device's memory properties and checks
 * for a memory type that matches the specified type filter and has the required properties (e.g., host visible, coherent).
 * If a suitable memory type is found, its index is returned. If no suitable memory type is found, an exception is thrown.
 */
uint32_t PathTracer::Impl::findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const bool matchesType = (typeFilter & (1u << i)) != 0;
        const bool matchesProps = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (matchesType && matchesProps) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable Vulkan memory type.");
}

/**
 * Creates a Vulkan instance, which is the connection between the application and the Vulkan library.
 * This function fills out a VkApplicationInfo structure with information about the application, and then uses it
 * to create a VkInstanceCreateInfo structure.
 */
void PathTracer::Impl::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanGPURT";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

/**
 * Picks a suitable physical device (GPU) that supports compute operations.
 * This function enumerates the available physical devices and checks each one for compute capabilities.
 */
void PathTracer::Impl::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-compatible GPU found.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    int bestScore = std::numeric_limits<int>::min();

    for (VkPhysicalDevice candidate : devices) {
        const std::optional<uint32_t> queueFamily = findComputeQueueFamily(candidate);
        if (!queueFamily.has_value()) {
            continue;
        }

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(candidate, &props);

        int score = 0;
        if (props.vendorID == 0x10DE) {
            score += 1000000;
        }
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 10000;
        }
        score += static_cast<int>(props.limits.maxComputeWorkGroupInvocations);

        if (score > bestScore) {
            bestScore = score;
            physicalDevice_ = candidate;
            physicalDeviceProperties_ = props;
            computeQueueFamilyIndex_ = queueFamily.value();
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable compute-capable GPU.");
    }

    std::cout << "Selected GPU: " << physicalDeviceProperties_.deviceName
              << " (vendor 0x" << std::hex << physicalDeviceProperties_.vendorID << std::dec << ")\n";

    if (physicalDeviceProperties_.vendorID != 0x10DE) {
        std::cout << "NVIDIA GPU not available as suitable compute device. Using best fallback.\n";
    }
}

/**
 * Creates a logical device, which is a connection to the physical device.
 * This function specifies the queue families to be used (in this case, the compute queue family) and the features to be enabled.
 */
void PathTracer::Impl::createLogicalDevice() {
    const float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex_;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &features;

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device.");
    }

    vkGetDeviceQueue(device_, computeQueueFamilyIndex_, 0, &computeQueue_);
}

/**
 * Creates a command pool and allocates a command buffer from it. The command pool is used to manage the memory for command buffers,
 * and the command buffer is used to record commands that will be submitted to the compute queue for execution.
 */
void PathTracer::Impl::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = computeQueueFamilyIndex_;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool.");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer.");
    }
}

/**
 * Creates a buffer and allocates memory for it.
 */
void PathTracer::Impl::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer.");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device_, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice_, memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(device_, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to allocate buffer memory.");
    }

    if (vkBindBufferMemory(device_, buffer, memory, 0) != VK_SUCCESS) {
        vkDestroyBuffer(device_, buffer, nullptr);
        vkFreeMemory(device_, memory, nullptr);
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to bind buffer memory.");
    }
}

/**
 * Creates the buffers for the scene data, output image, and camera data.
 * This function calls createBuffer() to create each buffer and allocate memory for it.
 */
void PathTracer::Impl::createBuffers() {
    const VkDeviceSize sceneBufferSize = static_cast<VkDeviceSize>(sceneData_.size() * sizeof(GpuSphere));
    const VkDeviceSize outputBufferSize = static_cast<VkDeviceSize>(config_.width) * static_cast<VkDeviceSize>(config_.height) * sizeof(OutputPixel);
    const VkDeviceSize cameraBufferSize = static_cast<VkDeviceSize>(sizeof(GpuCamera));

    createBuffer(
        sceneBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sceneBuffer_,
        sceneBufferMemory_
    );

    createBuffer(
        outputBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer_,
        outputBufferMemory_
    );

    createBuffer(
        cameraBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        cameraBuffer_,
        cameraBufferMemory_
    );

    void* mappedScene = nullptr;
    if (vkMapMemory(device_, sceneBufferMemory_, 0, sceneBufferSize, 0, &mappedScene) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map scene buffer memory.");
    }

    std::memcpy(mappedScene, sceneData_.data(), static_cast<size_t>(sceneBufferSize));
    vkUnmapMemory(device_, sceneBufferMemory_);

    void* mappedCamera = nullptr;
    if (vkMapMemory(device_, cameraBufferMemory_, 0, cameraBufferSize, 0, &mappedCamera) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map camera buffer memory.");
    }

    std::memcpy(mappedCamera, &cameraData_, sizeof(GpuCamera));
    vkUnmapMemory(device_, cameraBufferMemory_);
}

/**
 * Creates the descriptor set layout for the path tracer. The descriptor set layout describes the types of resources (buffers, images, etc.)
 * that will be accessed by the shader and how they are organized.
 */
void PathTracer::Impl::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding sceneBinding{};
    sceneBinding.binding = 0;
    sceneBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sceneBinding.descriptorCount = 1;
    sceneBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding outputBinding{};
    outputBinding.binding = 1;
    outputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputBinding.descriptorCount = 1;
    outputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 2;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {sceneBinding, outputBinding, cameraBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout.");
    }
}

/**
 * Creates the descriptor pool for the path tracer. The descriptor pool is used to allocate descriptor sets, which are the actual bindings of
 * resources that will be used by the shader.
 */
void PathTracer::Impl::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 3;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool.");
    }
}

/**
 * Allocates a descriptor set from the descriptor pool and binds it to the descriptor set layout.
 */
void PathTracer::Impl::createDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout_;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set.");
    }
}

/**
 * Resolves the path to the shader file.
 */
std::filesystem::path PathTracer::Impl::resolveShaderPath() const {
    // The renderer is expected to be launched from the repository root. CMake compiles and copies the
    // SPIR-V binary next to the executable under VulkanGPURT/build/<config>/shaders/, so look there first.
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        cwd / "VulkanGPURT" / "build" / "Release" / "shaders" / "path_tracer.comp.spv",
        cwd / "VulkanGPURT" / "build" / "Debug" / "shaders" / "path_tracer.comp.spv",
        cwd / "shaders" / "path_tracer.comp.spv",
        cwd / ".." / "shaders" / "path_tracer.comp.spv",
        cwd / ".." / ".." / "shaders" / "path_tracer.comp.spv"
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    std::string message = "Failed to locate path_tracer.comp.spv. Checked:";
    for (const std::filesystem::path& candidate : candidates) {
        message += "\n - " + candidate.string();
    }
    throw std::runtime_error(message);
}

/**
 * Creates the compute pipeline for the path tracer. Reads the shader code from the file, creates a shader module, and then creates a compute pipeline
 * using that shader module.
 */
void PathTracer::Impl::createComputePipeline() {
    const std::vector<char> shaderCode = readBinaryFile(resolveShaderPath());
    if (shaderCode.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Shader binary size must be a multiple of 4 bytes.");
    }

    std::vector<uint32_t> alignedCode(shaderCode.size() / sizeof(uint32_t));
    std::memcpy(alignedCode.data(), shaderCode.data(), shaderCode.size());

    VkShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = shaderCode.size();
    shaderModuleInfo.pCode = alignedCode.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module.");
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shaderModule, nullptr);
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = shaderModule;
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStage;
    pipelineInfo.layout = pipelineLayout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shaderModule, nullptr);
        throw std::runtime_error("Failed to create compute pipeline.");
    }

    vkDestroyShaderModule(device_, shaderModule, nullptr);
}

/**
 * Updates the descriptor set with the actual buffer bindings. This function creates VkDescriptorBufferInfo structures for each buffer and then
 * uses vkUpdateDescriptorSets() to write the buffer bindings into the descriptor set. This allows the compute shader to access the buffers when it is executed.
 */
void PathTracer::Impl::updateDescriptorSet() {
    VkDescriptorBufferInfo sceneBufferInfo{};
    sceneBufferInfo.buffer = sceneBuffer_;
    sceneBufferInfo.offset = 0;
    sceneBufferInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo outputBufferInfo{};
    outputBufferInfo.buffer = outputBuffer_;
    outputBufferInfo.offset = 0;
    outputBufferInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.buffer = cameraBuffer_;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> writes{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &sceneBufferInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &outputBufferInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &cameraBufferInfo;

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

}  // namespace vkgpu
