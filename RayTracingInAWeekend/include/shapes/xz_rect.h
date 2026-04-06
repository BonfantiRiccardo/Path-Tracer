#ifndef XZ_RECT_H
#define XZ_RECT_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * Axis-aligned rectangle in the XZ plane positioned at y = k.
 * Provides `sample_surface` and `area` helpers for area-light sampling.
 */
class xz_rect : public hittable {
public:
    xz_rect(double x0, double x1, double z0, double z1, double k, shared_ptr<material> mat, const vec3& normal = vec3(0,1,0))
        : x0(x0), x1(x1), z0(z0), z1(z1), k(k), mp(mat), normal(normal) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Solve for t where ray intersects plane y = k
        auto denom = r.direction().y();
        if (std::fabs(denom) < 1e-12) return false;
        double t = (k - r.origin().y()) / denom;
        if (!ray_t.surrounds(t)) return false;

        auto x = r.origin().x() + t * r.direction().x();
        auto z = r.origin().z() + t * r.direction().z();
        if (x < x0 || x > x1 || z < z0 || z > z1) return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.mat = mp;
        return true;
    }

    double area() const override { return (x1 - x0) * (z1 - z0); }

    // Uniformly sample a point on the rectangle surface and return the PDF and surface normal
    bool sample_surface(point3 &p, vec3 &normal_out, double &pdf) const override {
        double sx = random_double(x0, x1);
        double sz = random_double(z0, z1);
        p = point3(sx, k, sz);
        normal_out = normal;
        pdf = 1.0 / area();
        return true;
    }

    shared_ptr<material> get_material() const override { return mp; }

private:
    double x0, x1, z0, z1, k;
    shared_ptr<material> mp;
    vec3 normal;
};

#endif
