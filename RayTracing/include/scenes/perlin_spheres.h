#ifndef PERLIN_SPHERES_H
#define PERLIN_SPHERES_H

#include "scene_headers.h"

inline scene perlin_spheres_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;
    auto& world = result.world;

    auto pertext = make_shared<noise_texture>(4);       // Change the scale factor to adjust the frequency of the noise pattern

    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = image_width;             //400
    result.cam.samples_per_pixel = samples_per_pixel;       //100
    result.cam.max_depth         = max_depth;               //50
    result.cam.background        = color(0.70, 0.80, 1.00);

    result.cam.vfov     = 20;
    result.cam.lookfrom = point3(13,2,3);
    result.cam.lookat   = point3(0,0,0);
    result.cam.viewup   = vec3(0,1,0);

    result.cam.defocus_angle = 0;

    return result;
}

#endif // PERLIN_SPHERES_H