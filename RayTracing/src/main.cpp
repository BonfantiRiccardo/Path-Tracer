#include "../include/scenes/all_scenes.h"
#include "../include/bvh_node.h"
#include "../include/mesh/model_paths.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {

// Every dispatchable scene has the signature: scene <name>_scene(width, spp, depth).
using scene_builder = scene (*)(int, int, int);

// SCENE_ENTRY(foo) maps the scene file name "foo" to the function foo_scene, so
// scenes are selected on the command line by their exact file name (in
// include/scenes/) and the dispatcher never has to spell out each function by
// hand. Adding a scene is a one-line entry here; its header just has to define
// <name>_scene(width, spp, depth).
#define SCENE_ENTRY(name) { #name, &name##_scene }

const std::unordered_map<std::string, scene_builder>& scene_registry() {
    static const std::unordered_map<std::string, scene_builder> registry = {
        SCENE_ENTRY(ferrari_1987),
        SCENE_ENTRY(cornell_box),
        SCENE_ENTRY(cornell_smoke),
        SCENE_ENTRY(room_interior),
        SCENE_ENTRY(next_week),
        SCENE_ENTRY(abstract_life),
        SCENE_ENTRY(bouncing_spheres),
        SCENE_ENTRY(weekend),
        SCENE_ENTRY(checkered_spheres),
        SCENE_ENTRY(perlin_spheres),
        SCENE_ENTRY(quads),
        SCENE_ENTRY(light),
        SCENE_ENTRY(earth),
    };
    return registry;
}

#undef SCENE_ENTRY

void print_usage(std::ostream& out) {
    out << "Usage: main [options]\n"
        << "  -s, --scene <name>   Render a built-in scene by its file name (default: cornell_box).\n"
        << "      --mesh <path>    Render a single .obj/.gltf/.glb file. The path is tried as given\n"
        << "                       (relative to the working directory or absolute), then relative to\n"
        << "                       the models/ folder. Overrides --scene.\n"
        << "      --width <N>      Image width in pixels (default: 800).\n"
        << "      --spp <N>        Samples per pixel (default: 1000).\n"
        << "      --depth <N>      Maximum ray bounces (default: 100).\n"
        << "  -h, --help           Show this message.\n"
        << "\nScenes:";
    for (const auto& entry : scene_registry()) {
        out << ' ' << entry.first;
    }
    out << '\n';
}

scene select_scene(const std::string& scene_name, int image_width, int samples_per_pixel, int max_depth) {
    const auto& registry = scene_registry();
    const auto entry = registry.find(scene_name);
    if (entry != registry.end()) {
        return entry->second(image_width, samples_per_pixel, max_depth);
    }

    std::cerr << "Scene '" << scene_name << "' is not available.\n";
    print_usage(std::cerr);
    std::cerr << "Falling back to cornell_box.\n";
    return cornell_box_scene(image_width, samples_per_pixel, max_depth);
}

int parse_int(const char* text, int fallback) {
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::string scene_name = "cornell_box";
    std::string mesh_path;
    int image_width = 800;
    int samples_per_pixel = 1000;
    int max_depth = 100;
    bool width_set = false, spp_set = false, depth_set = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        // Consume the value that must follow a flag; report a clear error if it
        // is missing instead of silently ignoring the flag.
        const auto take_value = [&](std::string& out) -> bool {
            if (index + 1 < argc) {
                out = argv[++index];
                return true;
            }
            std::cerr << "Error: missing value for " << argument << "\n\n";
            print_usage(std::cerr);
            return false;
        };

        if (argument == "--help" || argument == "-h") {
            print_usage(std::cout);
            return 0;
        } else if (argument == "--scene" || argument == "-s") {
            if (!take_value(scene_name)) return 1;
        } else if (argument == "--mesh") {
            if (!take_value(mesh_path)) return 1;
        } else if (argument == "--width") {
            std::string value;
            if (!take_value(value)) return 1;
            image_width = parse_int(value.c_str(), image_width);
            width_set = true;
        } else if (argument == "--spp") {
            std::string value;
            if (!take_value(value)) return 1;
            samples_per_pixel = parse_int(value.c_str(), samples_per_pixel);
            spp_set = true;
        } else if (argument == "--depth") {
            std::string value;
            if (!take_value(value)) return 1;
            max_depth = parse_int(value.c_str(), max_depth);
            depth_set = true;
        } else {
            std::cerr << "Error: unknown argument '" << argument << "'\n\n";
            print_usage(std::cerr);
            return 1;
        }
    }

    // Resolve assets relative to the executable, which CMake places next to
    // mirrored copies of models/ and images/. This is independent of the working
    // directory the program is launched from.
    std::filesystem::path output_image = std::filesystem::path("out") / "image.ppm";
    if (argc > 0) {
        const std::filesystem::path executable_path = std::filesystem::absolute(argv[0]);
        if (executable_path.has_parent_path()) {
            const std::filesystem::path executable_dir = executable_path.parent_path();
            output_image = executable_dir / "image.ppm";
            set_models_dir(executable_dir / "models");
            set_images_dir(executable_dir / "images");
        }
    }

    scene current_scene;
    if (mesh_path.empty()) {
        current_scene = select_scene(scene_name, image_width, samples_per_pixel, max_depth);
    } else {
        // Accept a path as given (relative to the working directory or absolute);
        // if that does not exist, interpret it relative to the models/ folder.
        std::filesystem::path resolved(mesh_path);
        if (!std::filesystem::exists(resolved)) {
            resolved = model_path(mesh_path);
        }
        current_scene = mesh_scene(resolved);
    }

    // CLI render-setting overrides. mesh_scene picks its own defaults, so only
    // override a camera setting when the flag was actually passed.
    if (width_set) current_scene.cam.image_width = image_width;
    if (spp_set)   current_scene.cam.samples_per_pixel = samples_per_pixel;
    if (depth_set) current_scene.cam.max_depth = max_depth;

    current_scene.world = hittable_list(make_shared<bvh_node>(current_scene.world));

    current_scene.cam.render(current_scene.world, current_scene.lights, output_image);

    return 0;
}
