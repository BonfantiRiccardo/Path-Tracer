#include "path_tracer_impl.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace vkgpu {

/**
 * Constructor for the PathTracer class. 
 * Initializes the implementation with the provided RenderConfig.
 */
PathTracer::PathTracer(RenderConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

PathTracer::~PathTracer() = default;

PathTracer::PathTracer(PathTracer&&) noexcept = default;

PathTracer& PathTracer::operator=(PathTracer&&) noexcept = default;

void PathTracer::run() {
    impl_->run();
}

PathTracer::Impl::Impl(RenderConfig config)
    : config_(std::move(config)) {}

PathTracer::Impl::~Impl() {
    cleanup();
}

/**
 * Main function that runs the path tracing process. 
 * It initializes the scene and camera data, sets up Vulkan resources, 
 * dispatches the compute shader, and writes the output image to a file.
 */
void PathTracer::Impl::run() {
    std::srand(config_.seed);

    sceneDescription_ = buildSceneByName(config_.sceneName);
    if (sceneDescription_.spheres.empty()) {
        throw std::runtime_error("Scene is empty. Add at least one sphere.");
    }

    sceneData_ = uploadableScene(sceneDescription_.spheres);
    cameraData_ = uploadableCamera(sceneDescription_.camera);

    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createBuffers();
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
    createComputePipeline();
    updateDescriptorSet();

    dispatch();
    writePpmImage();

    std::cout << "Saved image to: " << config_.outputPath.string() << "\n";
}

}  // namespace vkgpu
