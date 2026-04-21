#ifndef CONE_H
#define CONE_H

#include "../raytracing.h"
#include "../hittable.h"
#include "plane.h"

/**
 * A cone class that inherits from the hittable interface. It represents a cone in 3D space and implements the hit function to determine if 
 * a ray intersects with it. 
 */
class cone : public hittable {
  public:
    cone(const point3& center, double radius, point3 apex, shared_ptr<material> mat) : center(center), radius(std::fmax(0,radius)), apex(apex), mat(mat) {
        vec3 height = center - apex;
        double height_len = height.length();

        if (height_len < 1e-8) {
            // Skip degenerate zero-height cones.
            bbox = bvh_aabb::empty;
            return;
        }

        vec3 height_norm = height / height_len;

        // Compute the bounding box of the cone by finding the maximum extent in the x, y, z directions 
        // based on the radius and height of the cone. Formula: r * sqrt(1 - (height_norm.x)^2)
        double x_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - height_norm.x()*height_norm.x()));
        double y_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - height_norm.y()*height_norm.y()));
        double z_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - height_norm.z()*height_norm.z()));

        point3 bbox_min(
            std::fmin(apex.x(), center.x() - x_extent),
            std::fmin(apex.y(), center.y() - y_extent),
            std::fmin(apex.z(), center.z() - z_extent)
        );
        point3 bbox_max(
            std::fmax(apex.x(), center.x() + x_extent),
            std::fmax(apex.y(), center.y() + y_extent),
            std::fmax(apex.z(), center.z() + z_extent)
        );
        bbox = bvh_aabb(bbox_min, bbox_max);

        base = make_shared<plane>(center, height_norm, mat); // Base plane of the cone facing downwards
    }
    
    /**
     * Cone equation is: x^2 + z^2 = k^2 * y^2, where k = r/h. To find the intersection of a ray with the cone, 
     * we substitute the ray equation P(t) = P0 + t*V into the cone equation and solve for t, which gives a quadratic 
     * equation with coefficients:
     * A = V.x^2 + V.z^2 - k^2 * V.y^2
     * B = 2(oc.x*V.x + oc.z*V.z - k^2 * oc.y*V.y)
     * C = oc.x^2 + oc.z^2 - k^2 * oc.y^2
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 height = center - apex;
        if (height.length() < 1e-8) return false; // Degenerate cone (height is zero)
        vec3 height_norm = unit_vector(height);

        double k = radius / height.length(); // k = h/r

        // Compute the coefficients of the quadratic equation for intersection with the cone's curved surface
        vec3 oc = r.origin() - apex;

        double oc_parallel = dot(oc, height_norm);
        vec3 oc_perp = oc - oc_parallel * height_norm;

        double dir_parallel = dot(r.direction(), height_norm);
        vec3 dir_perp = r.direction() - dir_parallel * height_norm;

        double A = dot(dir_perp, dir_perp)      - k*k * dir_parallel*dir_parallel;
        double H = dot(oc_perp, dir_perp)       - k*k * oc_parallel * dir_parallel;
        double C = dot(oc_perp, oc_perp)        - k*k * oc_parallel * oc_parallel;

        auto discriminant = H*H - A*C;

        // Step 1: Check for intersection with the curved surface of the cone by solving the quadratic equation
        hit_record curved_rec; // To store hit record for curved surface if hit occurs
        auto rootFlag = false; // Flag to indicate if a valid hit on the curved surface
        if (discriminant >= 0 && std::fabs(A) > 1e-8) {
            auto sqrtd = std::sqrt(discriminant);

            // Find the nearest root that lies in the acceptable range.
            auto root = (-H - sqrtd) / A;

            // Height along cone axis at hit point
            double y_local = oc_parallel + root * dir_parallel;

            if (ray_t.surrounds(root) && y_local >= 0 && y_local <= height.length()) {
                curved_rec.t = root;
                curved_rec.p = r.at(curved_rec.t);
                // Compute normal vector at the hit point using the gradient of the implicit cone equation
                double hit_parallel = dot(curved_rec.p - apex, height_norm);
                vec3 hit_perp = (curved_rec.p - apex) - hit_parallel * height_norm;

                vec3 outward_normal = unit_vector(hit_perp - k * k * hit_parallel * height_norm);

                curved_rec.set_face_normal(r, outward_normal);
                curved_rec.mat = mat;
                rootFlag = true; // Valid hit on the curved surface
            } else {
                root = (-H + sqrtd) / A;
                y_local = oc_parallel + root * dir_parallel;
                 if (ray_t.surrounds(root) && y_local >= 0 && y_local <= height.length()) {
                    curved_rec.t = root;
                    curved_rec.p = r.at(curved_rec.t);
                    // Compute normal vector at the hit point using the gradient of the implicit cone equation
                    double hit_parallel = dot(curved_rec.p - apex, height_norm);
                    vec3 hit_perp = (curved_rec.p - apex) - hit_parallel * height_norm;

                    vec3 outward_normal = unit_vector(hit_perp - k * k * hit_parallel * height_norm);

                    curved_rec.set_face_normal(r, outward_normal);
                    curved_rec.mat = mat;
                    rootFlag = true; // Valid hit on the curved surface
                }
            }

        }

        //Step 2: Check for intersection with the base of the cone (a plane) and see if it is closer
        hit_record base_rec;
        bool hit_base = base && base->hit(r, ray_t, base_rec);

        if (hit_base) {
            if ((base_rec.p - center).length_squared() > radius * radius) {
                hit_base = false;
            }
        }

        // Step 3: Return nearest valid hit
        if (hit_base && rootFlag) {
            rec = (base_rec.t < curved_rec.t) ? base_rec : curved_rec;
            return true;
        }

        if (hit_base) {
            rec = base_rec;
            return true;
        }

        if (rootFlag) {
            rec = curved_rec;
            return true;
        }

        return false; // No valid hit on either the curved surface or the base

    }

        bvh_aabb bounding_box() const override { return bbox; }

  private:
    point3 center;
    double radius;
    point3 apex;
    shared_ptr<plane> base = nullptr;
    shared_ptr<material> mat;
        bvh_aabb bbox;
    
        shared_ptr<material> get_material() const override { return mat; }
        double area() const override { return 0.0; }
        bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }
};

#endif
