#ifndef CORNELL_BOX_H
#define CORNELL_BOX_H

#include "scene_headers.h"

inline camera cornell_box_scene(hittable_list& world, int image_width, int samples_per_pixel, int max_depth) {

    auto red   = make_shared<lambertian>(color(.65, .05, .05));     // Red diffuse material (right wall)
    auto white = make_shared<lambertian>(color(.73, .73, .73));     // White diffuse material (floor, ceiling, back wall)
    auto green = make_shared<lambertian>(color(.12, .45, .15));     // Green diffuse material (left wall)
    auto light = make_shared<diffuse_light>(color(15, 15, 15));     // Bright white diffuse light (emissive material for the light source)

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = make_shared<aabb>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    world.add(box1);

    shared_ptr<hittable> box2 = make_shared<aabb>(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130, 0, 65));
    world.add(box2);


    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.viewup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    return cam;
}

#endif // CORNELL_BOX_H