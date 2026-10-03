#include "cli.h"
#include "path_tracer_app.h"

#include <cstdlib>
#include <exception>
#include <iostream>

/**
 * Entry point for the Vulkan GPU Path Tracer application.
 * Parses command-line arguments, initializes the path tracer, and runs the rendering process.
 * Catches and reports any exceptions that occur during execution.
 */
int main(int argc, char** argv) {
    try {
        const vkgpu::RenderConfig config = vkgpu::parseArguments(argc, argv);
        vkgpu::printRunBanner(config);      // Print configuration banner before starting the renderer

        vkgpu::PathTracer renderer(config);
        renderer.run();
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
