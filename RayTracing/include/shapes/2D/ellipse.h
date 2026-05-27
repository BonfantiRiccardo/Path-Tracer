#ifndef ELLIPSE_H
#define ELLIPSE_H

#include "../../raytracing.h"
#include "../../hittable.h"
#include "planar_primitive.h"

/**
 * An ellipse class that inherits from the hittable interface. It represents an ellipse in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs, using the Möller-Trumbore algorithm.
 */
class ellipse : public planar_primitive {
  public:
    ellipse(const point3& Q, const vec3& normal, double d1, double d2, shared_ptr<material> mat) 
                    : planar_primitive(Q, u_basis(normal, d1), v_basis(normal, d1, d2), mat), d1(d1), d2(d2) {
        set_bounding_box();
    }

    void set_bounding_box() override {
        // Compute the bounding box of the ellipse
        // Compute extent as projection of the ellipse vectors onto each axis
        const double ex = std::sqrt(u.x()*u.x() + v.x()*v.x());
        const double ey = std::sqrt(u.y()*u.y() + v.y()*v.y());
        const double ez = std::sqrt(u.z()*u.z() + v.z()*v.z());

        point3 pmin(Q.x() - ex, Q.y() - ey, Q.z() - ez);            // Mirrors the disk implementation, since the ellipse is just a scaled disk
        point3 pmax(Q.x() + ex, Q.y() + ey, Q.z() + ez);
        bbox = bvh_aabb(pmin, pmax);
    }


    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        // Check if the hit point is outside the ellipse using barycentric coordinates (a, b)
        if (a*a + b*b > 1.0)        // since u,v are already scaled by d1,d2; a,b will be normalized ellipse coordinates
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }

    private:
        double d1, d2; // semi-major and semi-minor axes of the ellipse
    
        static vec3 u_basis(const vec3& normal, double d1) {
            vec3 n = unit_vector(normal);
            vec3 u = cross(n, vec3(1, 0, 0)); // First spanning vector in the plane of the ellipse
            if (u.length_squared() < 1e-8) // If normal is parallel to (1, 0, 0), use a different vector to avoid zero cross product
                u = cross(n, vec3(0, 1, 0));
            return unit_vector(u) * d1;
        }

        static vec3 v_basis(const vec3& normal, double d1, double d2) {
            vec3 n = unit_vector(normal);
            vec3 v = cross(n, u_basis(normal, d1)); // Second spanning vector in the plane of the ellipse, orthogonal to the first
            return unit_vector(v) * d2;
        }
};

#endif // ELLIPSE_H