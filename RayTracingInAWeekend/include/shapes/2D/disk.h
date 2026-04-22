#ifndef DISK_H
#define DISK_H

#include "../../raytracing.h"
#include "../../hittable.h"
#include "planar_primitive.h"

/**
 * A disk class that inherits from the hittable interface. It represents a disk in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs, using the Möller-Trumbore algorithm.
 */
class disk : public planar_primitive {
  public:
    disk(const point3& Q, const vec3& normal, double radius, shared_ptr<material> mat) 
                    : normal(unit_vector(normal)), radius(radius),
                      planar_primitive(Q, u_basis(normal, radius), v_basis(normal, radius), mat) {

        set_bounding_box();
    }

    void set_bounding_box() override {
        // Compute the bounding box of the disk
        // max extent of disk in direction is projection of the disk vectors onto the axis
        const double ex = std::sqrt(u.x()*u.x() + v.x()*v.x());     // Pitagorean theorem to find half extent of disk
        const double ey = std::sqrt(u.y()*u.y() + v.y()*v.y());
        const double ez = std::sqrt(u.z()*u.z() + v.z()*v.z());

        point3 pmin(Q.x() - ex, Q.y() - ey, Q.z() - ez);
        point3 pmax(Q.x() + ex, Q.y() + ey, Q.z() + ez); 
        bbox = bvh_aabb(pmin, pmax);
    }


    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        // Check if the hit point is outside the disk using polar coordinates (a, b)
        if (a*a + b*b > 1.0)        // since u,v are already scaled by radius; a,b will be normalized disk coordinates
            return false;
        rec.u = a;
        rec.v = b;
        return true;
    }

    private:
        vec3 normal;
        double radius;

        static vec3 u_basis(const vec3& normal, double radius) {
            vec3 n = unit_vector(normal);
            vec3 u = cross(n, vec3(1, 0, 0)); // First spanning vector in the plane of the disk
            if (u.length_squared() < 1e-8) // If normal is parallel to (1, 0, 0), use a different vector to avoid zero cross product
                u = cross(n, vec3(0, 1, 0));
            return unit_vector(u) * radius;
        }

        static vec3 v_basis(const vec3& normal, double radius) {
            vec3 n = unit_vector(normal);
            vec3 u = u_basis(normal, radius);
            vec3 v = cross(n, u); // Second spanning vector in the plane of the disk, orthogonal to u
            return unit_vector(v) * radius;
        }
};

#endif // DISK_H