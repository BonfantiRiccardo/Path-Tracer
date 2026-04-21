#ifndef PLANE_H
#define PLANE_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * A plane class that inherits from the hittable interface. It represents an infinite plane in 3D space and implements the hit 
 * function to determine if a ray intersects with it.
 */
class plane : public hittable {
  public:
  plane(const point3& center, const vec3& normal, shared_ptr<material> mat) : center(center), normal(normal), mat(mat) {
    const double slab_half_thickness = 1e-4; // A small thickness that allows intersection with rays that are very close
    const double axis_alignment_eps = 1e-6; // A small threshold to determine if the plane is axis-aligned

     // If the normal vector is very close to zero, treat the plane as having an infinite bounding box.
    if (normal.length() < axis_alignment_eps) {
      bbox = bvh_aabb(point3(-infinity, -infinity, -infinity), point3(infinity, infinity, infinity));
      return;
    }

    vec3 unit_n = unit_vector(normal);
    // For axis-aligned planes, we can create a thin bounding box (slab) that extends infinitely 
    // in the two directions parallel to the plane and has a small thickness in the direction of the normal. 
    if (std::fabs(unit_n.x()) > 1.0 - axis_alignment_eps) {
      bbox = bvh_aabb(
        point3(center.x() - slab_half_thickness, -infinity, -infinity),
        point3(center.x() + slab_half_thickness, infinity, infinity)
      );
    } else if (std::fabs(unit_n.y()) > 1.0 - axis_alignment_eps) {
      bbox = bvh_aabb(
        point3(-infinity, center.y() - slab_half_thickness, -infinity),
        point3(infinity, center.y() + slab_half_thickness, infinity)
      );
    } else if (std::fabs(unit_n.z()) > 1.0 - axis_alignment_eps) {
      bbox = bvh_aabb(
        point3(-infinity, -infinity, center.z() - slab_half_thickness),
        point3(infinity, infinity, center.z() + slab_half_thickness)
      );
    } else {
      // A tilted infinite plane has an infinite axis-aligned bounding box.
      bbox = bvh_aabb(point3(-infinity, -infinity, -infinity), point3(infinity, infinity, infinity));
    }
  }

    /**
     * Plane equation is: N · (P - P0) = 0, where N is the normal vector of the plane, P0 is a point on the plane (center), and P is any point on the plane.
     * To find the intersection of a ray with the plane, we substitute the ray equation P(t) = A + t*B into the plane equation,
     * obtaining the following (after rearranging): t = N · (P0 - A) / N · B, where A is the ray origin and B is the ray direction.
     */
    bool hit(const ray &r, interval ray_t, hit_record &rec) const override
    {
        // Compute intersection
        auto cameraPos = r.origin();                    // The origin of the ray
        auto direction = r.direction();                 // The direction of the ray (B)
        auto planePoint = center;                       // The center point of the plane (P0)
        auto planeNormal = normal;                      // The normal vector of the plane (N)

        auto pointToEye = planePoint - cameraPos;       // Vector from ray origin to a point on the plane (P0 - A)

        auto numerator = dot(planeNormal, pointToEye);  // N · (P0 - A)
        auto denominator = dot(planeNormal, direction); // N · B


        const double epsilon = 1e-8;            // A small threshold to handle floating-point precision issues
        if (std::fabs(denominator) > epsilon)   // Check if the ray is not parallel to the plane
        {
            double t = numerator / denominator; // Compute t
            
            // Verify t is within the valid range of the ray
            if (!ray_t.surrounds(t))
                return false; // No valid intersection

            rec.t = t;
            rec.p = r.at(rec.t);                    // Compute the intersection point P(t) = A + t*B
            rec.set_face_normal(r, planeNormal);    // Set the normal vector at the hit point
            rec.mat = mat;                          // Set the material of the hit point
            
            return true;                            // Intersection occurred
        }

        // If no intersection, return null
        return false;
    }

    bvh_aabb bounding_box() const override { return bbox; }

  private:
    point3 center;
    vec3 normal;
    shared_ptr<material> mat;
    bvh_aabb bbox;
    
    shared_ptr<material> get_material() const override { return mat; }
    double area() const override { return 0.0; }
    bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }
};

#endif // PLANE_H