#ifndef SPHERE_H
#define SPHERE_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * A sphere class that inherits from the hittable interface. It represents a sphere in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs.
 */
class sphere : public hittable {
  public:
    // Stationary Sphere                                                                    center stays in position
    sphere(const point3& static_center, double radius, shared_ptr<material> mat) : center(static_center, vec3(0,0,0)), radius(std::fmax(0,radius)), mat(mat) 
    {
        auto rvec = vec3(radius, radius, radius);
        bbox = bvh_aabb(static_center - rvec, static_center + rvec);
    }

    // Moving Sphere                                                                                center moves linearly from c1 to c2
    sphere(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat) : center(center1, center2 - center1), radius(std::fmax(0,radius)), mat(mat) 
    {
        point3 rvec = vec3(radius, radius, radius);
        bvh_aabb box1(center.at(0) - rvec, center.at(0) + rvec);      // Bounding box that encloses the sphere at time 0 
        bvh_aabb box2(center.at(1) - rvec, center.at(1) + rvec);      // Bounding box that encloses the sphere at time 1
        bbox = bvh_aabb(box1, box2);
    }

    /**
     * Sphere equation is: (P - C) · (P - C) = r^2, where P is a point on the sphere, C is the center of the sphere, and r is the radius.
     * To find the intersection of a ray with the sphere, we substitute the ray equation P(t) = A + t*B into the sphere equation,
     * obtaining the following (after rearranging): t^2 * (B · B) + 2t * (B · (A - C)) + ((A - C) · (A - C) - r^2) = 0, which is a quadratic equation in t.
     * The coefficients of the quadratic equation are:
     * a = B · B
     * b = 2 * (B · (A - C))  --> simplify with h = B · (A - C) to avoid computing 2*b
     * c = (A - C) · (A - C) - r^2
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        point3 current_center = center.at(r.time());
        vec3 oc = current_center - r.origin();      // Vector from ray origin to sphere center (A - C)
        auto a = r.direction().length_squared();        // a = B · B
        auto h = dot(r.direction(), oc);                // h = B · (A - C) 
        auto c = oc.length_squared() - radius*radius;   // c = (A - C) · (A - C) - r^2

        auto discriminant = h*h - a*c;
        if (discriminant < 0)
            return false;           //No Hits

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;

        return true;
    }

    // Surface area of the sphere
    double area() const override { return 4.0 * pi * radius * radius; }

    static void get_sphere_uv(const point3& p, double& u, double& v) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = std::acos(-p.y());                 // y = -cos(theta)
        auto phi = std::atan2(-p.z(), p.x()) + pi;      // x = -cos(phi)*sin(theta)        z = sin(phi)*sin(theta)

        u = phi / (2*pi);
        v = theta / pi;
    }

    // Sample a (uniform) random point on the sphere surface, return point, normal and pdf
    bool sample_surface(point3 &p, vec3 &normal_out, double &pdf) const override {
        point3 current_center = center.at(0); // Assuming time 0 for static sampling
        p = current_center + radius * random_unit_vector();
        normal_out = unit_vector(p - current_center);
        pdf = 1.0 / area();
        return true;
    }

    shared_ptr<material> get_material() const override { return mat; }
    bvh_aabb bounding_box() const override { return bbox; }

    // Return the probability density function value for sampling a given direction from a point
    double pdf_value(const point3& origin, const vec3& direction) const override {
        // This method only works for stationary spheres.

        hit_record rec;
        if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
            return 0;

        auto dist_squared = (center.at(0) - origin).length_squared();
        auto cos_theta_max = std::sqrt(1 - radius*radius/dist_squared);
        auto solid_angle = 2*pi*(1-cos_theta_max);

        return  1 / solid_angle;
    }

    // Generate a random direction from a point towards the surface of the sphere using the appropriate function
    vec3 random(const point3& origin) const override {
        vec3 direction = center.at(0) - origin;
        auto distance_squared = direction.length_squared();
        onb uvw(direction);
        return uvw.transform(random_to_sphere(radius, distance_squared));
    }


  private:
    ray center;
    double radius;
    shared_ptr<material> mat;
    bvh_aabb bbox;

    // Generate a random direction from a point towards the surface of the sphere, uniformly distributed over the solid angle subtended by the sphere
    static vec3 random_to_sphere(double radius, double distance_squared) {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2*(std::sqrt(1-radius*radius/distance_squared) - 1);

        auto phi = 2*pi*r1;
        auto x = std::cos(phi) * std::sqrt(1-z*z);
        auto y = std::sin(phi) * std::sqrt(1-z*z);

        return vec3(x, y, z);
    }
};

#endif