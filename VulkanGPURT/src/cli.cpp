#include "cli.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace vkgpu {

namespace {

/**
 * Helper function to parse an unsigned integer from a string. 
 * Throws an exception if the value is invalid or out of range.
 */
uint32_t parseUInt(const std::string& value, const std::string& flagName) {
    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed > std::numeric_limits<uint32_t>::max()) {
            throw std::runtime_error("Value too large for " + flagName + ": " + value);
        }
        return static_cast<uint32_t>(parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid value for " + flagName + ": " + value);
    }
}

/**
 * Helper function to parse a double from a string. 
 * Throws an exception if the value is invalid or not finite.
 */
double parseDouble(const std::string& value, const std::string& flagName) {
    try {
        const double parsed = std::stod(value);
        if (!std::isfinite(parsed)) {
            throw std::runtime_error("Invalid value for " + flagName + ": " + value);
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid value for " + flagName + ": " + value);
    }
}

}  // namespace

/**
 * Prints usage information for the command-line interface.
 */
void printUsage() {
    const RenderConfig defaults{};

    std::cout
        << "Usage: vulkan_gpu_rt [options]\n"
        << "  --width <int>      Output width (default: " << defaults.width << ")\n"
        << "  --height <int>     Output height override (default: derived from width/aspect)\n"
        << "  --aspect <float>   Aspect ratio used to derive height (default: " << defaults.aspectRatio << ")\n"
        << "  --spp <int>        Samples per pixel (default: " << defaults.samplesPerPixel << ")\n"
        << "  --bounces <int>    Max bounces (default: " << defaults.maxBounces << ")\n"
        << "  --seed <int>       Random seed (default: " << defaults.seed << ")\n"
        << "  --scene <name>     Scene name: weekend | two-sphere\n"
        << "  --output <path>    Output PPM file path (default: " << defaults.outputPath.string() << ")\n"
        << "  --help             Show this help\n";
}

/**
 * Parses command-line arguments and returns a RenderConfig struct with the specified settings. 
 * Throws exceptions for invalid arguments or values.
 */
RenderConfig parseArguments(int argc, char** argv) {
    RenderConfig config{};
    bool heightProvided = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto requireValue = [&](const std::string& flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for " + flag);
            }
            return argv[++i];
        };

        if (arg == "--width") {
            config.width = parseUInt(requireValue(arg), arg);
        } else if (arg == "--height") {
            config.height = parseUInt(requireValue(arg), arg);
            heightProvided = true;
        } else if (arg == "--aspect") {
            config.aspectRatio = parseDouble(requireValue(arg), arg);
        } else if (arg == "--spp") {
            config.samplesPerPixel = parseUInt(requireValue(arg), arg);
        } else if (arg == "--bounces") {
            config.maxBounces = parseUInt(requireValue(arg), arg);
        } else if (arg == "--seed") {
            config.seed = parseUInt(requireValue(arg), arg);
        } else if (arg == "--scene") {
            config.sceneName = requireValue(arg);
        } else if (arg == "--output") {
            config.outputPath = requireValue(arg);
        } else if (arg == "--help") {
            printUsage();
            std::exit(EXIT_SUCCESS);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (config.width == 0 || config.height == 0) {
        throw std::runtime_error("Width and height must be greater than zero.");
    }
    if (config.aspectRatio <= 0.0) {
        throw std::runtime_error("Aspect ratio must be greater than zero.");
    }

    if (!heightProvided) {
        const double derivedHeight = static_cast<double>(config.width) / config.aspectRatio;
        const uint32_t clampedHeight = static_cast<uint32_t>(std::max(1.0, std::floor(derivedHeight)));
        config.height = clampedHeight;
    }
    if (config.samplesPerPixel == 0) {
        throw std::runtime_error("Samples per pixel must be greater than zero.");
    }
    if (config.maxBounces == 0) {
        throw std::runtime_error("Max bounces must be greater than zero.");
    }

    return config;
}

/**
 * Prints a banner with the current render configuration settings. 
 * This is called before starting the rendering process to inform the user of the parameters 
 * being used.
 */
void printRunBanner(const RenderConfig& config) {
    std::cout << "Vulkan GPU render\n"
              << "  resolution: " << config.width << "x" << config.height << "\n"
              << "  spp: " << config.samplesPerPixel << "\n"
              << "  max bounces: " << config.maxBounces << "\n"
              << "  scene: " << config.sceneName << "\n"
              << "  output: " << config.outputPath.string() << "\n";
}

}  // namespace vkgpu
