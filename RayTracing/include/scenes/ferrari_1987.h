#ifndef SCENE_FERRARI_1987_H
#define SCENE_FERRARI_1987_H

#include "scene_headers.h"
#include "../mesh/gltf_loader.h"
#include "../mesh/model_paths.h"
#include "../sky_light.h"

#include <filesystem>

inline scene ferrari_1987_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;

    // --- The car: scaled to ~4.4 m, turned 45 deg so a full side is visible,
    //     and lifted so the wheels rest on the asphalt (y = 0). ---
    const auto ferrari_model = model_path("1987_ferrari_f40/scene.gltf");
    auto ferrari = load_gltf_mesh(ferrari_model);
    if (ferrari) {
        shared_ptr<hittable> car = make_shared<uniform_scale>(ferrari, 100.0);
        car = make_shared<rotate_y>(car, 45.0);
        car = make_shared<translate>(car, vec3(0.0, 0.017, 0.0));
        result.world.add(car);
    }

    // --- Asphalt street under everything (base-color map, tiled). ---
    auto asphalt = make_shared<lambertian>(make_shared<uv_scale_texture>(
        make_shared<image_texture>("CityStreetAsphaltGenericClean001/CityStreetAsphaltGenericClean001_COL_2K.jpg"),
        8.0, 8.0));
    result.world.add(make_shared<quad>(point3(-20.0, 0.0, -20.0), vec3(0.0, 0.0, 40.0), vec3(40.0, 0.0, 0.0), asphalt));

    // --- Night sky: a large emissive dome sphere textured with the sky image,
    //     giving an image-based background instead of a flat color. ---
    result.world.add(make_shared<sphere>(
        point3(0.0, 0.0, 0.0), 1000.0,
        make_shared<sky_light>(make_shared<image_texture>("night-sky.png"), 1.0)));

    // --- Street furniture (brick wall, raised pavement, lamp post) built in a
    //     local frame that runs along local Z, then turned 45 deg so it stays
    //     PARALLEL to the car and slid out to the car's right. Grouping it under
    //     one rotate+translate keeps the wall and the continuous pavement strip
    //     aligned with each other. ---
    auto brick = make_shared<lambertian>(make_shared<uv_scale_texture>(
        make_shared<image_texture>("brick_wall.jpg"), 12.0, 3.0));
    auto pavement = make_shared<lambertian>(make_shared<uv_scale_texture>(
        make_shared<image_texture>("pavement.jpg"), 10.0, 1.0));
    auto curb = make_shared<lambertian>(color(0.28, 0.28, 0.27));
    auto pole_mat = make_shared<metal>(color(0.20, 0.20, 0.22), 0.15);

    const double curb_h = 0.15;
    auto street = make_shared<hittable_list>();
    // Brick wall running the length of the street (local Z), behind the pole.
    street->add(make_shared<quad>(point3(0.85, 0.0, -12.0), vec3(0.0, 0.0, 24.0), vec3(0.0, 5.5, 0.0), brick));
    // Continuous raised pavement: a textured top strip plus a curb edge facing the car.
    street->add(make_shared<quad>(point3(-0.8, curb_h + 0.001, -12.0), vec3(0.0, 0.0, 24.0), vec3(1.7, 0.0, 0.0), pavement));
    street->add(make_shared<quad>(point3(-0.8, 0.0, -12.0), vec3(0.0, 0.0, 24.0), vec3(0.0, curb_h, 0.0), curb));
    // Lamp post standing on the pavement.
    street->add(make_shared<cylinder>(point3(0.0, curb_h, 0.0), point3(0.0, 3.8, 0.0), 0.09, pole_mat));

    const vec3 street_offset(3.2, 0.0, -1.2);
    result.world.add(make_shared<translate>(make_shared<rotate_y>(street, 45.0), street_offset));

    // Small, bright, warm bulb sitting on top of the (rotated) pole. Placed in
    // world space directly so it can be importance-sampled as a light.
    const double bulb_radius = 0.20;
    auto bulb_mat = make_shared<diffuse_light>(color(160.0, 140.0, 98.0));
    auto bulb = make_shared<sphere>(point3(0.0, 3.8, 0.0) + street_offset + vec3(0.0, bulb_radius, 0.0), bulb_radius, bulb_mat);
    result.world.add(bulb);
    result.lights.add(bulb);

    result.cam.aspect_ratio = 16.0 / 9.0;
    result.cam.image_width = image_width;
    result.cam.samples_per_pixel = samples_per_pixel;
    result.cam.max_depth = max_depth;
    result.cam.background = color(0.0, 0.0, 0.0); // unused: the sky dome encloses the scene

    // Low camera, close to the car and tilted upward so the tall lamp fits.
    result.cam.vfov = 50;
    result.cam.lookfrom = point3(-0.3, 1.0, 5.8);
    result.cam.lookat = point3(0.7, 1.7, -0.8);
    result.cam.viewup = vec3(0, 1, 0);

    result.cam.defocus_angle = 0.0;
    result.cam.focus_dist = 6.6;

    return result;
}

#endif // SCENE_FERRARI_1987_H