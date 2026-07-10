#ifndef EARTH_SCENE_H
#define EARTH_SCENE_H

#include "scene_headers.h"

inline scene earth_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;
    auto& world = result.world;

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);
    world.add(globe);

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = image_width;             //400
    result.cam.samples_per_pixel = samples_per_pixel;             //100
    result.cam.max_depth         = max_depth;              //50
    result.cam.background        = color(0.70, 0.80, 1.00);

    result.cam.vfov     = 20;
    result.cam.lookfrom = point3(0,0,12);
    result.cam.lookat   = point3(0,0,0);
    result.cam.viewup      = vec3(0,1,0);

    result.cam.defocus_angle = 0;


    return result;
}

#endif // EARTH_SCENE_H