#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "../include/raytracing.h"
#include <vector>


/**
 * A list of hittable objects that can be intersected by a ray.
 */
class hittable_list : public hittable {
  public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = bvh_aabb(bbox, object->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                // record pointer to hit object for further queries (sampling/material)
                temp_rec.ptr = object;
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    bvh_aabb bounding_box() const override { return bbox; }

    private:
        bvh_aabb bbox; // Bounding box for the entire list, used for BVH construction
};

#endif