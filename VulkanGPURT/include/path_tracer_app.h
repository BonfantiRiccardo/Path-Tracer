#ifndef VULKAN_GPU_RT_PATH_TRACER_APP_H
#define VULKAN_GPU_RT_PATH_TRACER_APP_H

#include "path_tracer_types.h"

#include <memory>

namespace vkgpu {

/**
 * Main application class for the Vulkan GPU Path Tracer.
 */
class PathTracer {
public:
    explicit PathTracer(RenderConfig config);
    ~PathTracer();

    PathTracer(const PathTracer&) = delete;
    PathTracer& operator=(const PathTracer&) = delete;

    PathTracer(PathTracer&&) noexcept;
    PathTracer& operator=(PathTracer&&) noexcept;

    void run();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_PATH_TRACER_APP_H