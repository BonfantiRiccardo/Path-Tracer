#include "../include/scenes/shapes.h"

#include <filesystem>


int main(int argc, char* argv[]) {
    hittable_list world;
    camera cam;

    cam = build_shapes_scene(world);

    std::filesystem::path output_image = std::filesystem::path("out") / "image.ppm";
    if (argc > 0) {
        const std::filesystem::path executable_path = std::filesystem::absolute(argv[0]);
        if (executable_path.has_parent_path()) {
            output_image = executable_path.parent_path() / "image.ppm";
        }
    }

    cam.render(world, output_image);

    return 0;
}