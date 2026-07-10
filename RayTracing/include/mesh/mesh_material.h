#ifndef MESH_MATERIAL_H
#define MESH_MATERIAL_H

#include "../material.h"

/**
 * Modulates a texture by a constant color, component-wise. Used to apply glTF's
 * emissiveFactor * emissiveStrength on top of an emissive texture.
 */
class scaled_texture : public texture {
  public:
    scaled_texture(shared_ptr<texture> tex, const color& scale) : tex(tex), scale(scale) {}

    color value(double u, double v, const point3& p) const override {
        return scale * tex->value(u, v, p);
    }

  private:
    shared_ptr<texture> tex;
    color scale;
};

/**
 * A Lambertian surface that also emits light. This matches the glTF (and OBJ
 * `Ke`) convention where the emissive term is *added on top of* the base
 * shading rather than replacing it. It composites correctly because
 * `camera::ray_color` sums both contributions
 * (`color_from_emission + color_from_scatter`), so a material may both scatter
 * and emit. Using `diffuse_light` instead would turn the whole surface into a
 * pure emitter and lose the base color wherever the emissive map is black.
 */
class lambertian_emissive : public material {
  public:
    lambertian_emissive(shared_ptr<texture> base, shared_ptr<texture> emit)
      : base(base), emit(emit) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = base->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        auto cos_theta = dot(rec.normal, unit_vector(scattered.direction()));
        return cos_theta < 0 ? 0 : cos_theta / pi;
    }

    color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
        if (!rec.front_face)
            return color(0, 0, 0);
        return emit->value(u, v, p);
    }

  private:
    shared_ptr<texture> base;
    shared_ptr<texture> emit;
};

#endif // MESH_MATERIAL_H
