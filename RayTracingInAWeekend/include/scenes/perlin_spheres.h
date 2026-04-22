#ifndef PERLIN_SPHERES_H
#define PERLIN_SPHERES_H

#include "scene_headers.h"

inline camera perlin_spheres(hittable_list &world) {

    auto pertext = make_shared<noise_texture>(4);       // Change the scale factor to adjust the frequency of the noise pattern

    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.viewup   = vec3(0,1,0);

    cam.defocus_angle = 0;

    return cam;
}

#endif // PERLIN_SPHERES_H