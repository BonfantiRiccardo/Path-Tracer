#ifndef QUAD_H
#define QUAD_H

#include "../../raytracing.h"
#include "../../hittable.h"
#include "planar_primitive.h"

/**
 * 
 */
class quad : public planar_primitive {
  public:
    quad(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat) : planar_primitive(Q, u, v, mat) {
        surface_area = n.length();

        set_bounding_box();
    }

    void set_bounding_box() override {
        // Compute the bounding box of all four vertices.
        bvh_aabb bbox_diagonal1 = bvh_aabb(Q, Q + u + v);
        bvh_aabb bbox_diagonal2 = bvh_aabb(Q + u, Q + v);
        bbox = bvh_aabb(bbox_diagonal1, bbox_diagonal2);
    }


    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        if (!unit_interval.contains(a) || !unit_interval.contains(b))
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }

    // Return the probability density function value for sampling a given direction from a point
    double pdf_value(const point3& origin, const vec3& direction) const override {
        hit_record rec;
        if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
            return 0;

        auto distance_squared = rec.t * rec.t * direction.length_squared();
        auto cosine = std::fabs(dot(direction, rec.normal) / direction.length());

        return distance_squared / (cosine * surface_area);
    }

    // Generate a random direction from a point towards the surface of the primitive
    vec3 random(const point3& origin) const override {
        auto p = Q + (random_double() * u) + (random_double() * v);
        return p - origin;
    }
};

#endif // QUAD_H
