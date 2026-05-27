#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "../../raytracing.h"
#include "../../hittable.h"
#include "planar_primitive.h"

/**
 * A triangle class that inherits from the hittable interface. It represents a triangle in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs, using the Möller-Trumbore algorithm.
 */
class triangle : public planar_primitive {
  public:
    triangle(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat) : planar_primitive(Q, u, v, mat) {
        set_bounding_box();
    }

    void set_bounding_box() override {
        // Compute the bounding box of the triangle
        point3 bbox_min(            // Compute min corner of bounding box
            std::fmin(Q.x(), std::fmin(Q.x() + u.x(), Q.x() + v.x())),
            std::fmin(Q.y(), std::fmin(Q.y() + u.y(), Q.y() + v.y())),
            std::fmin(Q.z(), std::fmin(Q.z() + u.z(), Q.z() + v.z()))
        );
        point3 bbox_max(            // Compute max corner of bounding box
            std::fmax(Q.x(), std::fmax(Q.x() + u.x(), Q.x() + v.x())),
            std::fmax(Q.y(), std::fmax(Q.y() + u.y(), Q.y() + v.y())),
            std::fmax(Q.z(), std::fmax(Q.z() + u.z(), Q.z() + v.z()))
        );
        bbox = bvh_aabb(bbox_min, bbox_max);
    }


    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        // Check if the hit point is outside the triangle using barycentric coordinates (a, b)
        // The hit point is inside the triangle if a >= 0, b >= 0, and a + b <= 1.
        if (!unit_interval.contains(a) || !unit_interval.contains(b) || a + b > 1)
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }
};

#endif // TRIANGLE_H