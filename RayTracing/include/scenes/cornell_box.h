#ifndef CORNELL_BOX_H
#define CORNELL_BOX_H

#include "scene_headers.h"

inline scene cornell_box_scene(int image_width, int samples_per_pixel, int max_depth) {
    scene result;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));     // Red diffuse material (right wall)
    auto white = make_shared<lambertian>(color(.73, .73, .73));     // White diffuse material (floor, ceiling, back wall)
    auto green = make_shared<lambertian>(color(.12, .45, .15));     // Green diffuse material (left wall)
    auto light = make_shared<diffuse_light>(color(15, 15, 15));     // Bright white diffuse light (emissive material for the light source)

    // Left and right walls
    result.world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    result.world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));

    // Light
    auto ceiling_light = make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light);
    result.world.add(ceiling_light);
    result.lights.add(ceiling_light);

    auto empty_material = make_shared<material>();
    result.lights.add(make_shared<sphere>(point3(190, 90, 190), 90, empty_material));

    // Floor, ceiling, back wall
    result.world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    result.world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    result.world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    // Box
    shared_ptr<hittable> box1 = make_shared<aabb>(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    result.world.add(box1);

    // Box 1
    // shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
    // shared_ptr<hittable> box1 = make_shared<aabb>(point3(0,0,0), point3(165,330,165), aluminum);
    // box1 = make_shared<rotate_y>(box1, 15);
    // box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    // result.world.add(box1);

    // Box 2
    // shared_ptr<hittable> box2 = make_shared<aabb>(point3(0, 0, 0), point3(165, 165, 165), white);
    // box2 = make_shared<rotate_y>(box2, -18);
    // box2 = make_shared<translate>(box2, vec3(130, 0, 65));
    // result.world.add(box2);


    // Glass Sphere
    auto glass = make_shared<dielectric>(1.5);
    result.world.add(make_shared<sphere>(point3(190,90,190), 90, glass));

    result.cam.aspect_ratio      = 1.0;
    result.cam.image_width       = image_width;
    result.cam.samples_per_pixel = samples_per_pixel;
    result.cam.max_depth         = max_depth;
    result.cam.background        = color(0,0,0);

    result.cam.vfov     = 40;
    result.cam.lookfrom = point3(278, 278, -800);
    result.cam.lookat   = point3(278, 278, 0);
    result.cam.viewup   = vec3(0,1,0);

    result.cam.defocus_angle = 0;

    return result;
}

#endif // CORNELL_BOX_H
