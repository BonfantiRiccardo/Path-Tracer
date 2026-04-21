#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * A triangle class that inherits from the hittable interface. It represents a triangle in 3D space and implements the hit function to determine if a ray intersects with it. 
 * The hit function calculates the intersection point and normal vector at the hit point if an intersection occurs, using the Möller-Trumbore algorithm.
 */
class triangle : public hittable {
  public:
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat) : v0(v0), v1(v1), v2(v2), mat(mat) {
        const double min_side = 1e-6;       // Minimum side length to prevent degenerate bounding boxes for very small triangles

        // Compute the bounding box of the triangle by finding the minimum and maximum x, y, z coordinates among the three vertices.
        interval ix(
            std::fmin(v0.x(), std::fmin(v1.x(), v2.x())),
            std::fmax(v0.x(), std::fmax(v1.x(), v2.x()))
        );
        interval iy(
            std::fmin(v0.y(), std::fmin(v1.y(), v2.y())),
            std::fmax(v0.y(), std::fmax(v1.y(), v2.y()))
        );
        interval iz(
            std::fmin(v0.z(), std::fmin(v1.z(), v2.z())),
            std::fmax(v0.z(), std::fmax(v1.z(), v2.z()))
        );

        if (ix.size() < min_side) ix = ix.expand(min_side);
        if (iy.size() < min_side) iy = iy.expand(min_side);
        if (iz.size() < min_side) iz = iz.expand(min_side);

        bbox = bvh_aabb(ix, iy, iz);
    }

    /**
     * Triangle equation is: (P - v0) · (edge1 × edge2) = 0, where P is a point on the triangle, v0 is one of the triangle vertices, 
     * and edge1 and edge2 are the vectors along the edges of the triangle. To find the intersection of a ray with the triangle, we use the Möller-Trumbore algorithm.
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const double epsilon = 1e-8; // Small threshold to handle numerical precision issues

        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 h = cross(r.direction(), edge2);
        double a = dot(edge1, h);

        if (std::fabs(a) < epsilon)
            return false; // Ray is parallel to the triangle

        double f = 1.0 / a;
        vec3 s = r.origin() - v0;

        double u = f * dot(s, h);
        if (u < 0.0 || u > 1.0)
            return false; // Intersection point is outside the triangle

        vec3 q = cross(s, edge1);
        double v = f * dot(r.direction(), q);
        if (v < 0.0 || u + v > 1.0)
            return false; // Intersection point is outside the triangle

        // compute ray parameter t using Möller–Trumbore
        double t = f * dot(edge2, q);

        // require t to be in front of the ray origin and within the ray interval
        if (t <= epsilon || !ray_t.contains(t))
            return false;

        // fill hit record using t
        rec.t = t;
        rec.p = r.at(rec.t);

        // geometric normal (may be non-unit if triangle is degenerate)
        vec3 normal = cross(edge1, edge2);
        normal = normal / normal.length();
        rec.set_face_normal(r, normal);
        rec.mat = mat;

        return true;

    }

        bvh_aabb bounding_box() const override { return bbox; }

  private:
    point3 v0;
    point3 v1;
    point3 v2;
    shared_ptr<material> mat;
        bvh_aabb bbox;
    
        shared_ptr<material> get_material() const override { return mat; }
        double area() const override { return 0.0; }
        bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }
};

#endif // TRIANGLE_H