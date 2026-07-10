#ifndef VULKAN_GPU_RT_PATH_TRACER_TYPES_H
#define VULKAN_GPU_RT_PATH_TRACER_TYPES_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace vkgpu {

/**
 * Struct representing the push constants passed to the compute shader. 
 * Contains parameters for image dimensions, sphere count, rendering settings, and a random seed.
 */
struct PushConstants {
    uint32_t width;
    uint32_t height;
    uint32_t sphereCount;
    uint32_t samplesPerPixel;  // Number of samples traced in THIS dispatch (one sample batch), not the total.
    uint32_t maxBounces;
    uint32_t seed;
    uint32_t sampleOffset;     // Samples already accumulated before this batch; decorrelates each batch's RNG.
};

static_assert(sizeof(PushConstants) % 4 == 0, "Push constants must be 4-byte aligned.");

/**
 * Struct representing a sphere in GPU memory.
 * Aligned to 16 bytes for optimal memory access.
 */
struct alignas(16) GpuSphere {
    std::array<float, 4> centerRadius;
    std::array<float, 4> albedoMaterialType;
    std::array<float, 4> materialParams;
};

/**
 * Struct representing a camera in GPU memory.
 */
struct alignas(16) GpuCamera {
    std::array<float, 4> lookFromDefocusAngle;
    std::array<float, 4> lookAtFocusDist;
    std::array<float, 4> viewUpVfov;
};

/**
 * Struct representing a pixel in the output image. 
 * Each channel is a float to allow for high dynamic range values before tonemapping.
 */
struct OutputPixel {
    float r;
    float g;
    float b;
    float a;
};

/**
 * Struct representing the rendering configuration.
 */
struct RenderConfig {
    uint32_t width = 1200;
    uint32_t height = 675;
    double aspectRatio = 16.0 / 9.0;
    uint32_t samplesPerPixel = 20;
    uint32_t maxBounces = 10;
    uint32_t seed = 1;
    std::string sceneName = "weekend";
    std::filesystem::path outputPath = "render.ppm";
};

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_PATH_TRACER_TYPES_H