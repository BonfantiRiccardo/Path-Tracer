#ifndef HITTABLE_H
#define HITTABLE_H

#include "raytracing.h"

class material;

/**
 *  A record of a ray-object intersection, containing the relevant information
 */
class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    bool front_face;

    // Determines if the hit was on the front face or back face of the surface, and sets the normal vector accordingly.
    void set_face_normal(const ray& r, const vec3& outward_normal) {  //Outward normal assumed of unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

/**
 * An abstract class representing any object that can be hit by a ray.
 */
class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif