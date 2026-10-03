#ifndef PLANAR_PRIMITIVE_H
#define PLANAR_PRIMITIVE_H


#include "../../raytracing.h"
#include "../../hittable.h"

class planar_primitive : public hittable {
public:
    planar_primitive(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat) : Q(Q), u(u), v(v), mat(mat) {
        n = cross(u, v);                // Compute normal to the primitive
        normal = unit_vector(n);
        D = dot(normal, Q);             // Compute constant term D = N · P0 for the plane equation

        w = n / dot(n,n);               // Precompute w = N / (N · N) for efficient hit testing (used to get hit planar coordinates)

        surface_area = 0.0;             // Area will be computed in derived classes
    }

    /**
     * Hit is computed in 3 steps: 1) Find the plane that contains the primitive; 2) Check if the ray intersects the plane; 3) Check if the hit point lies inside the planar primitive
     * Plane equation is: N · (P - P0) = 0, where N is the normal vector of the plane, P0 is a point on the plane (center), and P is any point on the plane
     * To find the intersection of a ray with the plane, we substitute the ray equation P(t) = A + t*B into the plane equation,
     * obtaining the following (after rearranging): t = N · (P0 - A) / N · B, where A is the ray origin and B is the ray direction
     */
    bool hit(const ray &r, interval ray_t, hit_record &rec) const override {
        auto denom = dot(normal, r.direction()); // N · B

        // No hit if the ray is parallel to the plane.
        if (std::fabs(denom) < 1e-8)
            return false;

        // Return false if the hit point parameter t is outside the ray interval.
        auto t = (D - dot(normal, r.origin())) / denom;
        if (!ray_t.contains(t))
            return false;

        // Determine if the hit point lies within the planar shape using its plane coordinates.
        auto intersection = r.at(t);
        vec3 planar_hitpt_vector = intersection - Q;        // Vector from one corner of the primitive to the hit point:     p = P - Q
        auto alpha = dot(w, cross(planar_hitpt_vector, v)); // alpha coordinate of the hit point in the plane:          alpha = w · (p × v)
        auto beta = dot(w, cross(u, planar_hitpt_vector));  // beta coordinate of the hit point in the plane:           beta = w · (u × p)

        if (!is_interior(alpha, beta, rec))                 // Each primitive implements its own
            return false;

        // Ray hits the 2D shape; set the rest of the hit record and return true.
        rec.t = t;
        rec.p = intersection;
        rec.mat = mat;
        rec.set_face_normal(r, normal);

        return true;
    }

    bvh_aabb bounding_box() const override { return bbox; }

    shared_ptr<material> get_material() const override { return mat; }
    double area() const override { return surface_area; }
    bool sample_surface(point3 &p, vec3 &normal, double &pdf) const override { return false; }

protected:
    virtual bool is_interior(double a, double b, hit_record& rec) const = 0;
    virtual void set_bounding_box() = 0;

    point3 Q;
    vec3 u, v; // Not to be confused with texture coordinates, they are the two vectors spanning the primitive (its edges for quads and triangles)
    vec3 n, w;    // Precomputed vectors for hit testing: n = u × v, w = n / (n · n)
    shared_ptr<material> mat;
    bvh_aabb bbox;
    vec3 normal;
    double D; // Plane equation constant term
    double surface_area;
};


#endif // PLANAR_PRIMITIVE_H
