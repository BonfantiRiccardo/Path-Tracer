#ifndef SCENE_MESH_H
#define SCENE_MESH_H

#include "scene_headers.h"

#include "../mesh/gltf_loader.h"
#include "../mesh/obj_loader.h"

#include <filesystem>

inline scene mesh_scene(const std::filesystem::path& mesh_path) {
    scene result;

    auto extension = lower_case(mesh_path.extension().string());

    shared_ptr<triangle_mesh> mesh;
    if (extension == ".obj") {
        mesh = load_obj_mesh(mesh_path);
    } else if (extension == ".gltf" || extension == ".glb") {
        mesh = load_gltf_mesh(mesh_path);
    } else {
        std::cerr << "Unsupported mesh format: " << mesh_path.string() << '\n';
    }

    result.cam.aspect_ratio = 16.0 / 9.0;
    result.cam.image_width = 1200;
    result.cam.samples_per_pixel = 20;
    result.cam.max_depth = 10;
    result.cam.background = color(0.70, 0.80, 1.00);
    result.cam.vfov = 40;
    result.cam.viewup = vec3(0, 1, 0);
    result.cam.defocus_angle = 0.0;

    // Default framing, overridden below once we know the mesh's extent.
    result.cam.lookfrom = point3(0, 1.5, 4);
    result.cam.lookat = point3(0, 0.5, 0);
    result.cam.focus_dist = 4.0;

    if (mesh) {
        result.world.add(mesh);

        // Auto-frame the camera around the mesh so any model (of any authored
        // scale) is visible without hand-tuning the camera.
        const bvh_aabb bb = mesh->bounding_box();
        const point3 center(
            0.5 * (bb.x.min + bb.x.max),
            0.5 * (bb.y.min + bb.y.max),
            0.5 * (bb.z.min + bb.z.max));
        const double radius = 0.5 * vec3(
            bb.x.max - bb.x.min, bb.y.max - bb.y.min, bb.z.max - bb.z.min).length();
        if (radius > 0.0) {
            result.cam.lookat = center;
            result.cam.lookfrom = center + vec3(0.0, 0.4 * radius, 3.0 * radius);
            result.cam.focus_dist = (result.cam.lookfrom - result.cam.lookat).length();
        }
    }

    return result;
}

#endif // SCENE_MESH_H