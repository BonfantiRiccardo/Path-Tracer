#ifndef SCENE_LIGHT_H
#define SCENE_LIGHT_H

#include "scene_headers.h"

inline scene simple_light() {
    scene result;
    auto& world = result.world;


    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    auto light_quad = make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight);
    world.add(light_quad);
    result.lights.add(light_quad);

    auto light_sphere = make_shared<sphere>(point3(0,7,0), 2, difflight);
    world.add(light_sphere);
    result.lights.add(light_sphere);

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = 400;
    result.cam.samples_per_pixel = 100;
    result.cam.max_depth         = 50;
    result.cam.background        = color(0,0,0);

    result.cam.vfov     = 20;
    result.cam.lookfrom = point3(26,3,6);
    result.cam.lookat   = point3(0,2,0);
    result.cam.viewup   = vec3(0,1,0);

    result.cam.defocus_angle = 0;

    return result;
}

#endif // SCENE_LIGHT_H