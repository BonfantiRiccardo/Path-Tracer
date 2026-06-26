#ifndef ROOM_INTERIOR_H
#define ROOM_INTERIOR_H

#include "scene_headers.h"

inline scene build_room_interior_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;
    auto& world = result.world;
    auto floorTexture = make_shared<uv_scale_texture>(
        make_shared<image_texture>("wood_floor_4k.blend/textures/wood_floor_diff_4k.jpg"),
        4.0, 4.0
    );
    auto wallTexture = make_shared<uv_scale_texture>(
        make_shared<image_texture>("decorative_wall_2-4K/wallpaper.jpg"),
        2.0, 2.0
    );
    auto closetTexture = make_shared<uv_scale_texture>(
        make_shared<image_texture>("closet.jpg"),
        1.0, 1.0
    );
    auto sunriseTexture = make_shared<uv_scale_texture>(
        make_shared<image_texture>("sunrise.jpg"),
        1.0, 1.0
    );
    auto rugTexture = make_shared<uv_scale_texture>(
        make_shared<image_texture>("rug.jpg"),
        1.0, 1.0
    );

    auto floorWood   = make_shared<lambertian>(floorTexture);
    auto wallPaint   = make_shared<lambertian>(wallTexture);
    auto closetWood  = make_shared<lambertian>(closetTexture);
    auto tableTop    = make_shared<lambertian>(color(0.95, 0.95, 0.94));
    auto ceilingLight = make_shared<diffuse_light>(color(6.0, 6.0, 5.8));

    auto tableLeg   = make_shared<metal>(color(0.92, 0.92, 0.95), 0.0);
    auto cupRed     = make_shared<lambertian>(color(0.78, 0.16, 0.18));
    auto bottleCap  = make_shared<lambertian>(color(0.15, 0.30, 0.70));
    auto bottleLabel = make_shared<lambertian>(color(0.94, 0.96, 0.98));
    auto rugFabric  = make_shared<lambertian>(rugTexture);
    auto baseboardPaint = make_shared<lambertian>(color(0.92, 0.92, 0.90));
    auto chairSeat  = make_shared<lambertian>(color(0.27, 0.29, 0.31));
    auto chairLeg   = make_shared<metal>(color(0.78, 0.80, 0.82), 0.03);

    auto paintingFrame = make_shared<lambertian>(color(0.30, 0.20, 0.12));
    auto sunriseArt = make_shared<lambertian>(sunriseTexture);

    auto bottleGlass = make_shared<dielectric>(1.5);

    // 1. Room shell built from quads so the textures have bounded UVs.
    world.add(make_shared<quad>(point3(-4.0, 0.0, -4.0), vec3(8.0, 0.0, 0.0), vec3(0.0, 0.0, 8.0), floorWood));
    world.add(make_shared<quad>(point3(-4.0, 0.0, -4.0), vec3(8.0, 0.0, 0.0), vec3(0.0, 3.8, 0.0), wallPaint));
    world.add(make_shared<quad>(point3(-4.0, 0.0, -4.0), vec3(0.0, 0.0, 8.0), vec3(0.0, 3.8, 0.0), wallPaint));
    world.add(make_shared<quad>(point3(4.0, 0.0, -4.0), vec3(0.0, 0.0, 8.0), vec3(0.0, 3.8, 0.0), wallPaint));
    world.add(make_shared<quad>(point3(-4.0, 3.8, -4.0), vec3(8.0, 0.0, 0.0), vec3(0.0, 0.0, 8.0), wallPaint));

    // 1b. Baseboard trims along the room walls.
    world.add(make_shared<aabb>(point3(-4.0, 0.0, -4.0), point3(4.0, 0.08, -3.94), baseboardPaint));
    world.add(make_shared<aabb>(point3(-4.0, 0.0, -4.0), point3(-3.94, 0.08, 4.0), baseboardPaint));
    world.add(make_shared<aabb>(point3(3.94, 0.0, -4.0), point3(4.0, 0.08, 4.0), baseboardPaint));

    // 2. Closet behind the table (texture on all sides).
    const point3 closetMin(-3.8, 0.0, -3.95);
    const point3 closetMax(-2.0, 2.2, -3.05);
    world.add(make_shared<quad>(point3(closetMin.x(), closetMin.y(), closetMax.z()), vec3(closetMax.x() - closetMin.x(), 0.0, 0.0), vec3(0.0, closetMax.y() - closetMin.y(), 0.0), closetWood));
    world.add(make_shared<quad>(point3(closetMax.x(), closetMin.y(), closetMin.z()), vec3(closetMin.x() - closetMax.x(), 0.0, 0.0), vec3(0.0, closetMax.y() - closetMin.y(), 0.0), closetWood));
    world.add(make_shared<quad>(point3(closetMin.x(), closetMin.y(), closetMin.z()), vec3(0.0, 0.0, closetMax.z() - closetMin.z()), vec3(0.0, closetMax.y() - closetMin.y(), 0.0), closetWood));
    world.add(make_shared<quad>(point3(closetMax.x(), closetMin.y(), closetMax.z()), vec3(0.0, 0.0, closetMin.z() - closetMax.z()), vec3(0.0, closetMax.y() - closetMin.y(), 0.0), closetWood));
    world.add(make_shared<quad>(point3(closetMin.x(), closetMax.y(), closetMin.z()), vec3(closetMax.x() - closetMin.x(), 0.0, 0.0), vec3(0.0, 0.0, closetMax.z() - closetMin.z()), closetWood));
    world.add(make_shared<quad>(point3(closetMin.x(), closetMin.y(), closetMax.z()), vec3(closetMax.x() - closetMin.x(), 0.0, 0.0), vec3(0.0, 0.0, closetMin.z() - closetMax.z()), closetWood));

    // 3. Table: larger and closer to the camera.
    world.add(make_shared<aabb>(point3(-2.2, 0.86, -2.4), point3(2.2, 0.96, -0.2), tableTop));
    world.add(make_shared<cylinder>(point3(-1.9, 0.0, -2.15), point3(-1.9, 0.86, -2.15), 0.06, tableLeg));
    world.add(make_shared<cylinder>(point3( 1.9, 0.0, -2.15), point3( 1.9, 0.86, -2.15), 0.06, tableLeg));
    world.add(make_shared<cylinder>(point3(-1.9, 0.0, -0.45), point3(-1.9, 0.86, -0.45), 0.06, tableLeg));
    world.add(make_shared<cylinder>(point3( 1.9, 0.0, -0.45), point3( 1.9, 0.86, -0.45), 0.06, tableLeg));

    // 3b. Rug under the table.
    world.add(make_shared<quad>(point3(-2.6, 0.005, -2.9), vec3(5.3, 0.0, 0.0), vec3(0.0, 0.0, 3.3), rugFabric));

    // 3c. Chair moved to the left side of the table.
    world.add(make_shared<aabb>(point3(-3.35, 0.46, -1.85), point3(-2.45, 0.54, -0.95), chairSeat));
    world.add(make_shared<aabb>(point3(-3.35, 0.54, -1.85), point3(-3.25, 1.35, -0.95), chairSeat));
    world.add(make_shared<cylinder>(point3(-3.28, 0.0, -1.78), point3(-3.28, 0.46, -1.78), 0.03, chairLeg));
    world.add(make_shared<cylinder>(point3(-2.52, 0.0, -1.78), point3(-2.52, 0.46, -1.78), 0.03, chairLeg));
    world.add(make_shared<cylinder>(point3(-3.28, 0.0, -1.02), point3(-3.28, 0.46, -1.02), 0.03, chairLeg));
    world.add(make_shared<cylinder>(point3(-2.52, 0.0, -1.02), point3(-2.52, 0.46, -1.02), 0.03, chairLeg));

    // 4. Tableware: plastic cups and a transparent water bottle.
    world.add(make_shared<cylinder>(point3(-0.95, 0.96, -1.35), point3(-0.95, 1.18, -1.35), 0.07, cupRed));
    world.add(make_shared<cylinder>(point3(-0.55, 0.96, -1.05), point3(-0.55, 1.18, -1.05), 0.07, cupRed));
    world.add(make_shared<cylinder>(point3(-0.15, 0.96, -1.30), point3(-0.15, 1.18, -1.30), 0.07, cupRed));
    world.add(make_shared<cylinder>(point3(0.65, 0.96, -1.15), point3(0.65, 1.55, -1.15), 0.12, bottleGlass));
    world.add(make_shared<cylinder>(point3(0.65, 1.15, -1.15), point3(0.65, 1.30, -1.15), 0.122, bottleLabel));
    world.add(make_shared<cylinder>(point3(0.65, 1.55, -1.15), point3(0.65, 1.64, -1.15), 0.045, bottleCap));

    // 4b. Sunrise painting on the back wall.
    world.add(make_shared<quad>(point3(0.85, 1.35, -3.98), vec3(2.30, 0.0, 0.0), vec3(0.0, 1.35, 0.0), paintingFrame));
    world.add(make_shared<quad>(point3(1.00, 1.50, -3.975), vec3(2.00, 0.0, 0.0), vec3(0.0, 1.05, 0.0), sunriseArt));

    // 5. Ceiling area light (downward-facing)
    auto ceilingLightQuad = make_shared<quad>(point3(-2.0, 3.79, -2.0), vec3(4.0, 0, 0), vec3(0, 0, 4.0), ceilingLight);
    world.add(ceilingLightQuad);
    result.lights.add(ceilingLightQuad);

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = image_width;
    result.cam.samples_per_pixel = samples_per_pixel;
    result.cam.max_depth         = max_depth;
    result.cam.background        = color(0.62, 0.72, 0.82);

    // Viewed from human height, looking slightly downward into the room
    result.cam.vfov     = 42;
    result.cam.lookfrom = point3(2.4, 1.95, 4.4);
    result.cam.lookat   = point3(-0.3, 1.0, -1.4);
    result.cam.viewup   = vec3(0, 1, 0);

    result.cam.defocus_angle = 0.015;
    result.cam.focus_dist    = 5.9;
    return result;
}

#endif // ROOM_INTERIOR_H