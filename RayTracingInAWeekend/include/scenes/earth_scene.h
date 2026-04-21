#ifndef EARTH_SCENE_H
#define EARTH_SCENE_H

#include "scene_headers.h"

inline camera earth_scene(hittable_list &world) {

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);
    world.add(globe);

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat   = point3(0,0,0);
    cam.viewup      = vec3(0,1,0);

    cam.defocus_angle = 0;


    return cam;
}

#endif // EARTH_SCENE_H