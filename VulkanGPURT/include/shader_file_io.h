#ifndef VULKAN_GPU_RT_SHADER_FILE_IO_H
#define VULKAN_GPU_RT_SHADER_FILE_IO_H

#include <filesystem>
#include <vector>

namespace vkgpu {

/**
 * Reads the contents of a binary file into a vector of chars. Used for loading SPIR-V shader binaries.
 */
std::vector<char> readBinaryFile(const std::filesystem::path& filePath);

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_SHADER_FILE_IO_H