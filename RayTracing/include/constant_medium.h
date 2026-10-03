#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "hittable.h"
#include "material.h"
#include "texture.h"

// A hittable that represents a volume with constant density used to model effects like fog or smoke
// It is defined by a boundary (another hittable) and a density value
class constant_medium : public hittable {
public:
    constant_medium(shared_ptr<hittable> boundary, double density, shared_ptr<texture> tex)
        : boundary(boundary), neg_inv_density(-1/density),
          phase_function(make_shared<isotropic>(tex))
    {}

    constant_medium(shared_ptr<hittable> boundary, double density, const color& albedo)
        : boundary(boundary), neg_inv_density(-1/density),
          phase_function(make_shared<isotropic>(albedo))
    {}

    // Sample a scattering point inside the medium. Returns true if the ray scatters before leaving the boundary and stores that point in rec.
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record rec1, rec2;

        if (!boundary->hit(r, interval::universe, rec1))            // Check if the ray intersects the boundary of the medium (infinite interval to find entry point)
            return false;

        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2))   // Check for a second intersection to find the exit point (start just after the first hit to avoid self-intersection)
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;     // Make sure entry and exit point are within the ray's valid t interval
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)                           // Make sure entry point is before exit point
            return false;

        if (rec1.t < 0)                                 // If entry point is negative, clamp it to zero (ray starts inside the medium)
            rec1.t = 0;

        auto ray_length = r.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;     // Compute the distance the ray travels inside the medium
        auto hit_distance = neg_inv_density * std::log(random_double());    // Sample a random distance to the next scattering event based on an exponential distribution

        if (hit_distance > distance_inside_boundary)                        // If sampled scattering distance is > than distance to exit the medium, the ray exits without scattering
            return false;

        rec.t = rec1.t + hit_distance / ray_length;                         // Compute the t value along the ray where the scattering event occurs
        rec.p = r.at(rec.t);

        rec.normal = vec3(1,0,0);  // arbitrary
        rec.front_face = true;     // also arbitrary
        rec.mat = phase_function;

        return true;
    }

    bvh_aabb bounding_box() const override { return boundary->bounding_box(); }

private:
    shared_ptr<hittable> boundary;              // boundary is defined by a hittable object that encloses the volume (like sphere or box)
    double neg_inv_density;                     // negative inverse of the density, used to compute the distance to the next scattering event based on an exponential distribution
    shared_ptr<material> phase_function;        // isotropic material that models the scattering behavior of the medium
};

#endif // CONSTANT_MEDIUM_H