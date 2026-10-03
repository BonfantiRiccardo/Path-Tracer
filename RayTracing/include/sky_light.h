#ifndef SKY_LIGHT_H
#define SKY_LIGHT_H

#include "material.h"

/**
 * Emissive material for a background skydome: a very large sphere textured with
 * a sky image, centered on the scene. Unlike diffuse_light it emits on either
 * face (so it is visible from inside the enclosing dome) and it does not
 * scatter, so a ray that misses the scene geometry terminates on the sky
 * instead of the flat background color. `intensity` scales the emitted radiance
 * (a night sky wants a small value so it does not wash out the scene). Because
 * a sphere's UVs are an equirectangular map, the image wraps around as an
 * environment.
 */
class sky_light : public material {
public:
    sky_light(shared_ptr<texture> tex, double intensity = 1.0) : tex(tex), intensity(intensity) {}

    color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
        return intensity * tex->value(u, v, p);
    }

private:
    shared_ptr<texture> tex;
    double intensity;
};

#endif // SKY_LIGHT_H
