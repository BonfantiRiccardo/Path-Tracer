#ifndef ABSTRACT_LIFE_H
#define ABSTRACT_LIFE_H

#include "scene_headers.h"

inline scene abstract_life_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;
    auto& world = result.world;
    auto ground = make_shared<lambertian>(color(0.12, 0.12, 0.16));
    auto wall = make_shared<lambertian>(color(0.72, 0.86, 1.00));

    auto vividRed = make_shared<lambertian>(color(1.00, 0.10, 0.18));
    auto vividBlue = make_shared<lambertian>(color(0.06, 0.44, 1.00));
    auto vividYellow = make_shared<lambertian>(color(1.00, 0.88, 0.05));
    auto vividMagenta = make_shared<lambertian>(color(1.00, 0.10, 0.92));

    auto chrome = make_shared<metal>(color(0.94, 0.97, 1.00), 0.0);
    auto gold = make_shared<metal>(color(0.96, 0.72, 0.14), 0.05);
    auto copper = make_shared<metal>(color(0.94, 0.49, 0.20), 0.10);
    auto tealMetal = make_shared<metal>(color(0.20, 0.90, 0.86), 0.04);

    auto glass = make_shared<dielectric>(1.5);
    auto denseGlass = make_shared<dielectric>(1.8);

    // 1. Planes (Ground + 2 Backdrop walls for an enclosed corner feel)
    world.add(make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), ground));
    world.add(make_shared<plane>(point3(0, 0, -5), vec3(0, 0, 1), wall));
    world.add(make_shared<plane>(point3(-6, 0, 0), vec3(1, 0, 0), wall));

    // 2. AABBs (Pedestals resting on the floor)
    world.add(make_shared<aabb>(point3(1.5, 0.0, -1.5), point3(3.0, 1.2, 0.0), copper));
    world.add(make_shared<aabb>(point3(-3.0, 0.0, -2.0), point3(-1.5, 0.6, -0.5), vividBlue));

    // 3. Spheres (Resting on the floor and on a pedestal)
    // Radius is 0.6, so y-center is 0.6 to touch the ground
    world.add(make_shared<sphere>(point3(0.0, 0.6, 1.0), 0.6, glass));
    // Resting on the copper AABB (top is y=1.2, radius 0.4 -> y-center 1.6)
    world.add(make_shared<sphere>(point3(2.2, 1.6, -0.7), 0.4, vividRed)); 

    // 4. Cylinders (One upright, one tilted/rolling)
    // Upright on the blue AABB (top is y=0.6)
    world.add(make_shared<cylinder>(point3(-2.2, 0.6, -1.2), point3(-2.2, 2.0, -1.2), 0.3, denseGlass));
    // Tilted: Lying flat on the ground. Centers are at y=0.25 (matching radius)
    world.add(make_shared<cylinder>(point3(-1.0, 0.25, 2.5), point3(1.5, 0.25, 1.8), 0.25, chrome));

    // 5. Cones (One upright, one tilted/knocked over)
    // Upright on the ground
    world.add(make_shared<cone>(point3(4.0, 0.0, 1.5), 0.8, point3(4.0, 2.2, 1.5), tealMetal));
    // Tilted: Lying flat on the ground. Center y matches radius, apex y is 0.
    world.add(make_shared<cone>(point3(-1.5, 0.5, 1.0), 0.5, point3(-3.5, 0.0, 2.5), gold));

    // 6. Triangles (Used as glass shards leaning against the copper AABB)
    world.add(make_shared<triangle>(point3(1.5, 0.0, 0.5), vec3(0.8, 0.0, 0.0), vec3(1.5, 0.8, -0.2), vividYellow));
    world.add(make_shared<triangle>(point3(1.5, 0.0, 1.0), vec3(2.2, 0.0, 1.2), vec3(1.5, 0.6, 0.5), vividMagenta));

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = image_width;        //1200
    result.cam.samples_per_pixel = samples_per_pixel;         //100
    result.cam.max_depth         = max_depth;          //10
    result.cam.background        = color(0.70, 0.80, 1.00);

    result.cam.vfov     = 40;
    result.cam.lookfrom = point3(0.0, 3.5, 8.0);
    result.cam.lookat   = point3(0.5, 0.8, 0.0);
    result.cam.viewup   = vec3(0, 1, 0);

    result.cam.defocus_angle = 0.02;
    result.cam.focus_dist    = 8.5;
    return result;
}

#endif // ABSTRACT_LIFE_H