#ifndef SPHERE_H
#define SPHERE_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * A sphere class that inherits from the hittable interface. It represents a sphere in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs.
 */
class sphere : public hittable {
  public:
    sphere(const point3& center, double radius, shared_ptr<material> mat) : center(center), radius(std::fmax(0,radius)), mat(mat) {}

    /**
     * Sphere equation is: (P - C) · (P - C) = r^2, where P is a point on the sphere, C is the center of the sphere, and r is the radius.
     * To find the intersection of a ray with the sphere, we substitute the ray equation P(t) = A + t*B into the sphere equation,
     * obtaining the following (after rearranging): t^2 * (B · B) + 2t * (B · (A - C)) + ((A - C) · (A - C) - r^2) = 0, which is a quadratic equation in t.
     * The coefficients of the quadratic equation are:
     * a = B · B
     * b = 2 * (B · (A - C))  --> simplify with h = B · (A - C) to avoid computing 2*b
     * c = (A - C) · (A - C) - r^2
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 oc = center - r.origin();      // Vector from ray origin to sphere center (A - C)
        auto a = r.direction().length_squared();        // a = B · B
        auto h = dot(r.direction(), oc);                // h = B · (A - C) 
        auto c = oc.length_squared() - radius*radius;   // c = (A - C) · (A - C) - r^2

        auto discriminant = h*h - a*c;
        if (discriminant < 0)
            return false;           //No Hits

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

    // Surface area of the sphere
    double area() const override { return 4.0 * pi * radius * radius; }

    // Sample a (uniform) random point on the sphere surface, return point, normal and pdf
    bool sample_surface(point3 &p, vec3 &normal_out, double &pdf) const override {
        p = center + radius * random_unit_vector();
        normal_out = unit_vector(p - center);
        pdf = 1.0 / area();
        return true;
    }

    shared_ptr<material> get_material() const override { return mat; }

  private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
};

#endif