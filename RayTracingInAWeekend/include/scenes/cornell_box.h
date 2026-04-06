#ifndef CORNELL_BOX_H
#define CORNELL_BOX_H

#include "scene_headers.h"

inline camera build_cornell_box_scene(hittable_list& world) {
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

    // 1. Planes (The Enclosed Cornell Box)
    // Floor
    world.add(make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), wall));
    // Ceiling (No light, just a solid plane)
    world.add(make_shared<plane>(point3(0, 5, 0), vec3(0, -1, 0), wall));
    // Back Wall
    world.add(make_shared<plane>(point3(0, 2.5, -2.5), vec3(0, 0, 1), wall));
    // Left Wall
    world.add(make_shared<plane>(point3(-2.5, 2.5, 0), vec3(1, 0, 0), vividRed));
    // Right Wall 
    world.add(make_shared<plane>(point3(2.5, 2.5, 0), vec3(-1, 0, 0), vividBlue));

    // 2. AABBs (The classic Cornell blocks, modified with your materials)
    // Tall block on the left
    world.add(make_shared<aabb>(point3(-1.8, 0.0, -1.8), point3(-0.6, 2.5, -0.6), chrome));
    // Short block on the right
    world.add(make_shared<aabb>(point3(0.5, 0.0, -1.0), point3(1.7, 1.2, 0.2), ground));

    // 3. Spheres
    // Resting perfectly on top of the short block (block y=1.2, radius=0.45)
    world.add(make_shared<sphere>(point3(1.1, 1.65, -0.4), 0.45, glass));
    // Resting on the floor near the front opening
    world.add(make_shared<sphere>(point3(-1.2, 0.4, 1.2), 0.4, denseGlass));

    // 4. Cylinders (One upright pedestal, one rolling on the floor)
    // Upright pedestal
    world.add(make_shared<cylinder>(point3(-0.5, 0.0, 0.8), point3(-0.5, 1.0, 0.8), 0.3, copper));
    // Tilted rolling cylinder (radius 0.2, y-centers at 0.2 so it rests flush on the floor)
    world.add(make_shared<cylinder>(point3(0.8, 0.2, 1.5), point3(2.0, 0.2, 1.8), 0.2, vividMagenta));

    // 5. Cones (One displayed upright, one toppled over)
    // Upright on the copper pedestal (base y=1.0)
    world.add(make_shared<cone>(point3(-0.5, 1.0, 0.8), 0.35, point3(-0.5, 1.8, 0.8), tealMetal));
    // Tilted cone on the floor (base center y=0.3, radius=0.3, apex y=0. The edge and tip both touch the floor)
    world.add(make_shared<cone>(point3(0.2, 0.3, 0.0), 0.3, point3(1.2, 0.0, -0.2), gold));

    // 6. Triangles
    // Leaning against the tall chrome box
    world.add(make_shared<triangle>(point3(-0.6, 0.0, -0.8), point3(-0.2, 0.0, -1.2), point3(-0.6, 1.0, -1.0), vividYellow));
    // Lying flat on top of the short box
    world.add(make_shared<triangle>(point3(0.6, 1.201, -0.8), point3(1.6, 1.201, -0.8), point3(1.1, 1.201, -0.1), vividRed));

    // Larger rectangular ceiling light to illuminate the Cornell box (area light)
    auto ceiling_light = make_shared<diffuse_light>(color(8.0, 8.0, 8.0));
    // x-range and z-range sized to fit inside the box; normal points downwards
    world.add(make_shared<xz_rect>(-1.0, 1.0, -1.0, 1.0, 4.6, ceiling_light, vec3(0, -1, 0)));

    camera cam;

    cam.aspect_ratio      = 1.0; // Classic Cornell Box renders are usually perfectly square (1:1)
    cam.image_width       = 600;
    cam.samples_per_pixel = 400;
    cam.max_depth         = 10;

    // Viewing from outside the open front of the box, looking directly in
    cam.vfov     = 45;
    cam.lookfrom = point3(0.0, 2.5, 6.5);
    cam.lookat   = point3(0.0, 2.0, 0.0);
    cam.viewup   = vec3(0, 1, 0);

    cam.defocus_angle = 0.0; // Usually zero for Cornell box to keep geometry sharp
    cam.focus_dist    = 6.5;
    return cam;
}

#endif // CORNELL_BOX_H