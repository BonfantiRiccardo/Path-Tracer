#ifndef VULKAN_GPU_RT_CLI_H
#define VULKAN_GPU_RT_CLI_H

#include "path_tracer_types.h"

namespace vkgpu {

RenderConfig parseArguments(int argc, char** argv);
void printUsage();
void printRunBanner(const RenderConfig& config);

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_CLI_H