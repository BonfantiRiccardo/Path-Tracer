#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"

class material {
  public:
    virtual ~material() = default;

  // Scatter incoming ray. Returns true if the ray is scattered and sets
  // `attenuation` and `scattered`. Default: no scattering.
  virtual bool scatter(
    const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
  ) const {
    return false;
  }

  // Emitted radiance from the material (for emissive materials). Default: black.
  virtual color emitted() const {
    return color(0,0,0);
  }
};

/**
 * Lambertian material that scatters rays in a random direction within the hemisphere oriented around the hit normal. 
 * The color of the scattered ray is determined by the albedo of the material.
 */
class lambertian : public material {
  public:
    lambertian(const color& albedo) : albedo(albedo) {}

    color get_albedo() const { return albedo; }

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
      // Cosine-weighted hemisphere sampling using an ONB built from the hit normal.
      vec3 w = rec.normal;
      vec3 a = (std::fabs(w.x()) > 0.9) ? vec3(0,1,0) : vec3(1,0,0);
      vec3 u = unit_vector(cross(a, w));
      vec3 v = cross(w, u);

      vec3 rd = random_cosine_direction(); // local-space sample (z is up)
      auto scatter_direction = u * rd.x() + v * rd.y() + w * rd.z();

      // Catch degenerate scatter direction
      if (scatter_direction.near_zero())
        scatter_direction = rec.normal;

      scattered = ray(rec.p, scatter_direction);
      attenuation = albedo;
      return true;
    }

  private:
    color albedo;
};


/** 
 * Metal material that reflects rays in a deterministic manner based on the hit normal.
 * The color of the reflected ray is determined by the albedo of the material, and a fuzz factor can be applied to create a blurred reflection effect.
 */
class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

  private:
    color albedo;
    double fuzz;
};

/**
 * Dielectric material that refracts rays based on the hit normal and the material's refractive index. 
 * The color of the refracted ray is determined by the attenuation factor (white for dielectrics). 
 * The scatter function computes the refracted ray direction using Snell's law and handles total internal reflection when necessary.
 */
class dielectric : public material {
  public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

  private:
    // Refractive index in vacuum, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media.
    double refraction_index;

    static double reflectance(double cosine, double refraction_index) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*std::pow((1 - cosine),5);
    }
};

/**
 * Simple diffuse emissive material. Does not scatter rays, but returns an
 * emitted color when hit.
 */
class diffuse_light : public material {
  public:
    diffuse_light(const color& c) : emit(c) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        return false;
    }

    color emitted() const override {
        return emit;
    }

  private:
    color emit;
};

#endif