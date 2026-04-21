#ifndef SCENE_CHECKERED_SPHERES_H
#define SCENE_CHECKERED_SPHERES_H

#include "scene_headers.h"

#include "scene_headers.h"

inline camera build_checkered_spheres_scene(hittable_list &world) {

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.viewup   = vec3(0,1,0);

    cam.defocus_angle = 0;

    return cam;
}

#endif // SCENE_CHECKERED_SPHERES_H