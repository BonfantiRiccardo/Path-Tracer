#ifndef ROOM_INTERIOR_H
#define ROOM_INTERIOR_H

#include "scene_headers.h"

inline camera build_room_interior_scene(hittable_list& world) {
    // Materials optimized for an interior setting
    auto floorWood  = make_shared<lambertian>(color(0.35, 0.25, 0.15));
    auto wallPaint  = make_shared<lambertian>(color(0.90, 0.90, 0.92));
    auto tableWood  = make_shared<lambertian>(color(0.60, 0.40, 0.20));
    auto ceilingLight = make_shared<diffuse_light>(color(7.0, 7.0, 7.0));
    
    auto rubberRed  = make_shared<lambertian>(color(0.80, 0.15, 0.20));
    auto paperWhite = make_shared<lambertian>(color(0.95, 0.95, 0.95));

    auto mirror     = make_shared<metal>(color(0.95, 0.95, 0.95), 0.01);
    auto brass      = make_shared<metal>(color(0.85, 0.65, 0.20), 0.05);
    auto vaseMetal  = make_shared<metal>(color(0.20, 0.60, 0.80), 0.15);

    auto clearGlass = make_shared<dielectric>(1.5);
    auto water      = make_shared<dielectric>(1.33);

    // 1. Planes (The Room: Floor, Back Wall, Left Wall)
    world.add(make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), floorWood));
    world.add(make_shared<plane>(point3(0, 0, -4), vec3(0, 0, 1), wallPaint));
    world.add(make_shared<plane>(point3(-4, 0, 0), vec3(1, 0, 0), wallPaint));

    // 2. AABBs (Tabletop and a Low Bookshelf)
    // Tabletop
    world.add(make_shared<aabb>(point3(-1.5, 1.2, -2.5), point3(1.5, 1.3, -0.5), tableWood));
    // Bookshelf pushed against back wall
    world.add(make_shared<aabb>(point3(-3.8, 0.0, -3.9), point3(-2.0, 2.0, -3.0), tableWood));

    // 3. Cylinders (Table legs + Leaning broom stick)
    // 4 Upright Table Legs
    world.add(make_shared<cylinder>(point3(-1.3, 0.0, -2.3), point3(-1.3, 1.2, -2.3), 0.06, brass));
    world.add(make_shared<cylinder>(point3( 1.3, 0.0, -2.3), point3( 1.3, 1.2, -2.3), 0.06, brass));
    world.add(make_shared<cylinder>(point3(-1.3, 0.0, -0.7), point3(-1.3, 1.2, -0.7), 0.06, brass));
    world.add(make_shared<cylinder>(point3( 1.3, 0.0, -0.7), point3( 1.3, 1.2, -0.7), 0.06, brass));
    
    // Tilted: A broomstick leaning against the right corner
    world.add(make_shared<cylinder>(point3(3.5, 0.03, -3.5), point3(2.5, 3.0, -3.8), 0.03, tableWood));
    // Tilted: A drinking glass knocked over on the table
    world.add(make_shared<cylinder>(point3(0.5, 1.35, -1.0), point3(1.0, 1.35, -1.5), 0.1, clearGlass));

    // 4. Cones (Vase on table + Party hat on the floor)
    // Upright: Vase on the table (inverted cone look)
    world.add(make_shared<cone>(point3(-0.5, 1.3, -1.5), 0.15, point3(-0.5, 2.2, -1.5), vaseMetal));
    // Tilted: Party hat resting on its side on the floor
    world.add(make_shared<cone>(point3(1.5, 0.2, 1.0), 0.2, point3(2.5, 0.0, 0.5), rubberRed));

    // 5. Spheres (Decorative glass ball + rubber toy ball)
    // Glass ball on the bookshelf (Shelf top is y=2.0)
    world.add(make_shared<sphere>(point3(-2.9, 2.3, -3.4), 0.3, clearGlass));
    // Rubber ball rolling on the floor
    world.add(make_shared<sphere>(point3(-1.0, 0.25, 1.5), 0.25, rubberRed));
    // Water drop spilled on the table
    world.add(make_shared<sphere>(point3(0.8, 1.32, -1.2), 0.04, water));

    // 6. Triangles (A folded paper airplane on the floor)
    // Left wing
    world.add(make_shared<triangle>(point3(-0.5, 0.01, 2.5), vec3(0.5, 0.01, 2.8), vec3(0.0, 0.15, 2.8), paperWhite));
    // Right wing
    world.add(make_shared<triangle>(point3( 0.5, 0.01, 3.1), vec3(-0.5, 0.01, 2.5), vec3(0.0, 0.15, 2.8), paperWhite));

    // 7. Ceiling area light (downward-facing)
    world.add(make_shared<quad>(point3(-2.0, 4.99, -2.0), vec3(4.0, 0, 0), vec3(0, 0, 4.0), ceilingLight));

    camera cam;
    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 1200;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 5;
    cam.background        = color(0.70, 0.80, 1.00);

    // Viewed from human height, looking slightly downward into the room
    cam.vfov     = 50;
    cam.lookfrom = point3(2.0, 2.8, 5.0);
    cam.lookat   = point3(-0.5, 1.0, -1.5);
    cam.viewup   = vec3(0, 1, 0);

    cam.defocus_angle = 0.01;
    cam.focus_dist    = 7.0;
    return cam;
}

#endif // ROOM_INTERIOR_H