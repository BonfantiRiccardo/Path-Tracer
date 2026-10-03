#include "shader_file_io.h"

#include <fstream>
#include <stdexcept>
#include <string>

namespace vkgpu {

/**
 * Reads the contents of a binary file into a vector of chars. Used for loading SPIR-V shader binaries.
 */
std::vector<char> readBinaryFile(const std::filesystem::path& filePath) {
    // Open the file in binary mode and position the cursor at the end to determine the file size
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath.string());
    }

    // Get the size of the file by checking the position of the cursor at the end
    const std::streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        throw std::runtime_error("File is empty: " + filePath.string());
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    return buffer;
}

}  // namespace vkgpu
