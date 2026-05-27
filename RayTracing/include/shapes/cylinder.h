#ifndef CYLINDER_H
#define CYLINDER_H

#include "../raytracing.h"
#include "../hittable.h"
#include "2D/plane.h"

/**
 * A cylinder class that inherits from the hittable interface. It represents a cylinder in 3D space 
 * defined by two center points (the centers of the circular caps) and a radius.
 */
class cylinder : public hittable {
  public:
    cylinder(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat) : center1(center1), center2(center2), radius(std::fmax(0,radius)), mat(mat) {
        vec3 a = center2 - center1;
        double height = a.length();

        if (height < 1e-8) {
            // Skip degenerate zero-height cylinders.
            bbox = bvh_aabb::empty;
            return;
        }

        vec3 anorm = a / height;

        // Compute the bounding box of the cylinder by finding the maximum extent in the x, y, z directions
        // based on the radius and the orientation of the cylinder axis. Formula: r * sqrt(1 - (anorm.x)^2)
        double x_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - anorm.x()*anorm.x()));
        double y_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - anorm.y()*anorm.y()));
        double z_extent = radius * std::sqrt(std::fmax(0.0, 1.0 - anorm.z()*anorm.z()));

        point3 bbox_min(
            std::fmin(center1.x(), center2.x()) - x_extent,
            std::fmin(center1.y(), center2.y()) - y_extent,
            std::fmin(center1.z(), center2.z()) - z_extent
        );
        point3 bbox_max(
            std::fmax(center1.x(), center2.x()) + x_extent,
            std::fmax(center1.y(), center2.y()) + y_extent,
            std::fmax(center1.z(), center2.z()) + z_extent
        );
        bbox = bvh_aabb(bbox_min, bbox_max);

        bottom = make_shared<plane>(center1, -anorm, mat);
        top = make_shared<plane>(center2, anorm, mat);
    }

    /**
     * Cylinder equation is: 
     *     - axis: a = c1 - c0
     *     - normalized axis direction: anorm = a / |a|
     *     - height: h = |a|
     * To find the intersection of a ray with the cylinder, we substitute the ray equation P(t) = P0 + t*V into the cylinder equation, 
     * which can be expressed in terms of the components of the ray and the cylinder axis:
     *    - OC = P0 - c0       -->       OCperp = OC - (OC · anorm) anorm
     *    - Dperp = V - (V · anorm) anorm
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        vec3 a = center2 - center1;                                         // Cylinder axis
        double height = a.length();
        if (height < 1e-8) return false;                                // Degenerate cylinder (height is zero)
        vec3 anorm = a / height;                                        // Normalized cylinder axis direction
        vec3 oc = r.origin() - center1;                                     // Vector from ray origin to cylinder base center
        vec3 oc_perp = oc - dot(oc, anorm) * anorm;                         // Remove component along cylinder axis
        vec3 d_perp = r.direction() - dot(r.direction(), anorm) * anorm;    // Remove component along cylinder axis
        
        // Step 1: Solve quadratic equation for curved surface intersection (equation is |OCperp + t*Dperp|^2 = r^2)
        auto A = d_perp.length_squared();                               // A = Dperp · Dperp 
        auto H = dot(oc_perp, d_perp);                                  // H = OCperp · Dperp
        auto C = dot(oc_perp, oc_perp) - radius * radius;               // C = OCperp · OCperp - r^2

        auto discriminant = H*H - A*C;
        auto rootFlag = false;
        hit_record curved_rec; // To store hit record for curved surface if hit occurs

        const double epsilon = 1e-8; // Small threshold to handle numerical precision issues
        if (discriminant >= 0 && A > epsilon) {
            // Find roots of the quadratic equation
            auto sqrt_disc = std::sqrt(discriminant);

            auto root = (- H - sqrt_disc) / A; // First root (potentially closer hit)

            // Find the distance along the cylinder axis
            auto m = dot(r.at(root) - center1, anorm); // m = (P - c0) · anorm

            if (ray_t.surrounds(root) && m >= 0 && m <= height) {
                // Valid hit on the curved surface
                curved_rec.t = root;
                curved_rec.p = r.at(curved_rec.t);
                
                vec3 p_axis = center1 + m * anorm; // Projection of hit point onto cylinder axis
                vec3 outward_normal = (curved_rec.p - p_axis) / radius; // Outward normal at the hit point
                curved_rec.set_face_normal(r, outward_normal);
                curved_rec.mat = mat;
                rootFlag = true;
            } else {
                root = (- H + sqrt_disc) / A; // Second root

                m = dot(r.at(root) - center1, anorm);

                if (ray_t.surrounds(root) && m >= 0 && m <= height) {
                    // Valid hit on the curved surface
                    curved_rec.t = root;
                    curved_rec.p = r.at(curved_rec.t);
                    
                    vec3 p_axis = center1 + m * anorm; // Projection of hit point onto cylinder axis
                    vec3 outward_normal = (curved_rec.p - p_axis) / radius; // Outward normal at the hit point
                    curved_rec.set_face_normal(r, outward_normal);
                    curved_rec.mat = mat;
                    rootFlag = true;
                }
            }
        }

        // Step 2: Check for cap intersections (plane-ray intersection with the two caps), use plane header
        // Bottom cap plane: (P - c0) · anorm = 0
        // Top cap plane: (P - c1) · anorm = 0
        hit_record rec_bottom, rec_top;
        bool hit_bottom = bottom->hit(r, ray_t, rec_bottom);
        bool hit_top = top->hit(r, ray_t, rec_top);

        // Check if the hit points on the caps are within the circular area of the caps
        if (hit_bottom) {
            vec3 v = rec_bottom.p - center1;
            vec3 radial = v - dot(v, anorm) * anorm;
            if (radial.length_squared() > radius*radius) hit_bottom = false;
        }
        if (hit_top) {
            vec3 v = rec_top.p - center2;
            vec3 radial = v - dot(v, anorm) * anorm;
            if (radial.length_squared() > radius*radius) hit_top = false;
        }

        // Determine the closest valid hit among the curved surface and the caps
        hit_record closest_rec;
        
        if (hit_bottom && (!hit_top || rec_bottom.t < rec_top.t) && (!rootFlag || rec_bottom.t < curved_rec.t)) {
            closest_rec = rec_bottom;
        } else if (hit_top && (!hit_bottom || rec_top.t < rec_bottom.t) && (!rootFlag || rec_top.t < curved_rec.t)) {
            closest_rec = rec_top;
        } else if (rootFlag) {
            closest_rec = curved_rec;
        } else {
            return false; // No valid hit on either the curved surface or the caps
        }

        rec = closest_rec; // Set the output hit record to the closest hit
        return true;
    }

        bvh_aabb bounding_box() const override { return bbox; }

  private:
    point3 center1;
    point3 center2;
    shared_ptr<plane> bottom = nullptr;
    shared_ptr<plane> top = nullptr;
    double radius;
    shared_ptr<material> mat;
        bvh_aabb bbox;
    
    shared_ptr<material> get_material() const override { return mat; }
    double area() const override { return 0.0; }
    bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }
};

#endif