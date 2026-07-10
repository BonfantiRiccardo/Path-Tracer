#ifndef UNIFORM_SCALE_H
#define UNIFORM_SCALE_H

#include "../hittable.h"

/**
 * Uniformly scales a wrapped hittable about the origin by a constant factor.
 * This is the missing companion to `translate` and `rotate_y`, and is needed
 * because imported meshes rarely arrive in the scene's unit scale (e.g. a glTF
 * car authored ~100x too small).
 *
 * The ray direction is scaled by 1/factor so the hit parameter t (and hence the
 * distance ordering and the interpolated shading normal, which is unaffected by
 * uniform scale) is preserved; only the hit point is scaled back into world
 * space.
 */
class uniform_scale : public hittable {
  public:
    uniform_scale(shared_ptr<hittable> object, double factor)
      : object(object), factor(factor), inv_factor(1.0 / factor) {
        const bvh_aabb bb = object->bounding_box();
        bbox = bvh_aabb(
            point3(bb.x.min * factor, bb.y.min * factor, bb.z.min * factor),
            point3(bb.x.max * factor, bb.y.max * factor, bb.z.max * factor)
        );
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const ray scaled_r(r.origin() * inv_factor, r.direction() * inv_factor, r.time());
        if (!object->hit(scaled_r, ray_t, rec))
            return false;

        rec.p = rec.p * factor;
        return true;
    }

    bvh_aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    double factor;
    double inv_factor;
    bvh_aabb bbox;
};

#endif // UNIFORM_SCALE_H
