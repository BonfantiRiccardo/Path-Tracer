#ifndef HITTABLE_H
#define HITTABLE_H

#include "BVH_AABB.h"

class material;
class hittable; // forward declare so hit_record can hold a shared_ptr

/**
 *  A record of a ray-object intersection, containing the relevant information
 */
class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    double u;
    double v;
    bool front_face;
    shared_ptr<hittable> ptr; // pointer back to the hit object

    // Determines if the hit was on the front face or back face of the surface, and sets the normal vector accordingly.
    void set_face_normal(const ray& r, const vec3& outward_normal) {  //Outward normal assumed of unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

/**
 * An abstract class representing any object that can be hit by a ray.
 */
class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    // Return the material associated with this hittable (if known).
    virtual shared_ptr<material> get_material() const { return nullptr; }

    // Area of the surface (0 for non-area primitives).
    virtual double area() const { return 0.0; }

    // Sample a point on the surface (uniformly) and return the surface normal and pdf.
    // Return true if sampling is supported for this primitive.
    virtual bool sample_surface(point3 &p, vec3 &normal, double &pdf) const { return false; }

    // Return the axis-aligned bounding box of the primitive for BVH construction.
    virtual bvh_aabb bounding_box() const = 0;

    // Return the probability density function value for sampling a given direction from a point.
    virtual double pdf_value(const point3& origin, const vec3& direction) const { return 0.0; }

    // Generate a random direction from a point towards the surface of the primitive.
    virtual vec3 random(const point3& origin) const { return vec3(1,0,0); }

    // Whether this hittable contains no objects (only meaningful for containers
    // such as hittable_list; used to skip light sampling when there are no lights).
    virtual bool empty() const { return false; }
};

// A hittable that translates another hittable by a given offset
class translate : public hittable {
  public:
    translate(shared_ptr<hittable> object, const vec3& offset) : object(object), offset(offset)
    {
        bbox = object->bounding_box() + offset;
    }


    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Move the ray backwards by the offset
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }

    bvh_aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    vec3 offset;
    bvh_aabb bbox;
};


class rotate_y : public hittable {
  public:
    rotate_y(shared_ptr<hittable> object, double angle) : object(object) {
        // Init sin and cos variables and empty bbox
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        // Init min and max so that they can be updated in the loop
        point3 min( infinity,  infinity,  infinity);
        point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {                   // Loop over the 8 corners of the original bounding box
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;       // Compute the coordinates of the corner point (x, y, z) based on the current combination of i, j, k
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;       // Compute the new x and z coordinates of the corner point after rotation around y-axis by given angle
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {               // Update the minimum and maximum coordinates of the bounding box to include the new corner point after rotation
                        min[c] = std::fmin(min[c], tester[c]);
                        max[c] = std::fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = bvh_aabb(min, max);
    }
    

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        // Transform the ray from world space to object space.
        auto origin = point3(
            (cos_theta * r.origin().x()) - (sin_theta * r.origin().z()),            // x′ = cos(theta) * x - sin(theta) * z
            r.origin().y(),
            (sin_theta * r.origin().x()) + (cos_theta * r.origin().z())             // z′ = sin(theta) * x + cos(theta) * z
        );

        auto direction = vec3(
            (cos_theta * r.direction().x()) - (sin_theta * r.direction().z()),
            r.direction().y(),
            (sin_theta * r.direction().x()) + (cos_theta * r.direction().z())
        );

        ray rotated_r(origin, direction, r.time());

        // Determine whether an intersection exists in object space (and if so, where).
        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Transform the intersection from object space back to world space (inverse w.r.t. previous)
        rec.p = point3(
            (cos_theta * rec.p.x()) + (sin_theta * rec.p.z()),          // x′ = cos(theta) * x + sin(theta) * z
            rec.p.y(),
            (-sin_theta * rec.p.x()) + (cos_theta * rec.p.z())          // z′ = −sin(theta) * x + cos(theta) * z 
        );

        rec.normal = vec3(
            (cos_theta * rec.normal.x()) + (sin_theta * rec.normal.z()),
            rec.normal.y(),
            (-sin_theta * rec.normal.x()) + (cos_theta * rec.normal.z())
        );

        return true;
    }


    bvh_aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    bvh_aabb bbox;
};

#endif // HITTABLE_H
