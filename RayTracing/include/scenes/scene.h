#ifndef SCENE_H
#define SCENE_H

#include "../camera.h"
#include "../hittable_list.h"

struct scene {
    hittable_list world;
    hittable_list lights;
    camera cam;
};

#endif // SCENE_H
