#ifndef ANNULUS_H
#define ANNULUS_H

#include "../../raytracing.h"
#include "../../hittable.h"
#include "planar_primitive.h"

/**
 * An annulus class that inherits from planar_primitive. It represents an annulus in 3D space.
 * planar_primitive::hit computes the intersection point and normal. This class only tests whether the hit lies between the inner and outer radius.
 */
class annulus : public planar_primitive {
public:
    annulus(const point3& Q, const vec3& normal, double inner_radius, double outer_radius, shared_ptr<material> mat)
                    : normal(unit_vector(normal)), inner_radius(inner_radius), outer_radius(outer_radius),
                      planar_primitive(Q, u_basis(normal, outer_radius), v_basis(normal, outer_radius), mat) {
        set_bounding_box();
    }

    void set_bounding_box() override {
        // Compute the bounding box of the annulus
        const double ex = std::sqrt(u.x()*u.x() + v.x()*v.x());
        const double ey = std::sqrt(u.y()*u.y() + v.y()*v.y());
        const double ez = std::sqrt(u.z()*u.z() + v.z()*v.z());

        point3 pmin(Q.x() - ex, Q.y() - ey, Q.z() - ez);            // Mirrors the disk implementation, since the annulus fits inside its outer disk
        point3 pmax(Q.x() + ex, Q.y() + ey, Q.z() + ez);
        bbox = bvh_aabb(pmin, pmax);
    }


    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        // Check if the hit point is outside the annulus using coordinates (a, b)
        // since u,v are already scaled by outer_radius; a,b will be normalized annulus coordinates
        // Compute the radius of the hit point in the plane, scaling back up to the actual size of the annulus
        const double r = std::sqrt(a*a + b*b) * outer_radius;
        if (r < inner_radius || r > outer_radius)
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }


private:
    vec3 normal;
    double inner_radius, outer_radius; // inner and outer radius of the annulus

    static vec3 u_basis(const vec3& normal, double radius) {
        vec3 n = unit_vector(normal);
        vec3 u = cross(n, vec3(1, 0, 0)); // First spanning vector in the plane of the annulus
        if (u.length_squared() < 1e-8) // If normal is parallel to (1, 0, 0), use a different vector to avoid zero cross product
            u = cross(n, vec3(0, 1, 0));
        return unit_vector(u) * radius;
    }

    static vec3 v_basis(const vec3& normal, double radius) {
        vec3 n = unit_vector(normal);
        vec3 v = cross(n, u_basis(normal, radius)); // Second spanning vector in the plane of the annulus, orthogonal to the first
        return unit_vector(v) * radius;
    }
};

#endif // ANNULUS_H