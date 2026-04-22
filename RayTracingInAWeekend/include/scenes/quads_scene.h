#ifndef QUADS_SCENE_H
#define QUADS_SCENE_H

#include "scene_headers.h"

inline camera quads_scene(hittable_list &world) {
    
    // Materials
    auto left_red     = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    // Quads
    world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
    world.add(make_shared<disk>(point3(0, 0, 0), vec3(0, 0, 1), 2.0, back_green));
    world.add(make_shared<ellipse>(point3(3, 0, 3), vec3(1, 0, 0), 2.0, 3.0, right_blue));
    world.add(make_shared<annulus>(point3(0, 3, 3), vec3(0, 1, 0), 1.5, 2.0, upper_orange));
    world.add(make_shared<triangle>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat   = point3(0,0,0);
    cam.viewup   = vec3(0,1,0);

    cam.defocus_angle = 0;

    return cam;
}

#endif // QUADS_SCENE_H