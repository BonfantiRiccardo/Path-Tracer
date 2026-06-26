#ifndef SCENE_CHECKERED_SPHERES_H
#define SCENE_CHECKERED_SPHERES_H

#include "scene_headers.h"

#include "scene_headers.h"
inline scene build_checkered_spheres_scene() {
    scene result;
    auto& world = result.world;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    result.cam.aspect_ratio      = 16.0 / 9.0;
    result.cam.image_width       = 400;
    result.cam.samples_per_pixel = 100;
    result.cam.max_depth         = 50;
    result.cam.background        = color(0.70, 0.80, 1.00);


    result.cam.vfov     = 20;
    result.cam.lookfrom = point3(13,2,3);
    result.cam.lookat   = point3(0,0,0);
    result.cam.viewup   = vec3(0,1,0);

    result.cam.defocus_angle = 0;

    return result;
}

#endif // SCENE_CHECKERED_SPHERES_H