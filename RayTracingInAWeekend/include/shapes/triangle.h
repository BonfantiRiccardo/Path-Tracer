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
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat) : v0(v0), v1(v1), v2(v2), mat(mat) {}

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

  private:
    point3 v0;
    point3 v1;
    point3 v2;
    shared_ptr<material> mat;
    
        shared_ptr<material> get_material() const override { return mat; }
        double area() const override { return 0.0; }
        bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }
};

#endif


/*
Triangle

Triangles are essential if eventually you want to render meshes or OBJ files.

Use the ray-triangle intersection formula with barycentric coordinates.

Given triangle vertices 
𝑣
0
,
𝑣
1
,
𝑣
2
v
0
	​

,v
1
	​

,v
2
	​

:

𝑒
1
=
𝑣
1
−
𝑣
0
e
1
	​

=v
1
	​

−v
0
	​

𝑒
2
=
𝑣
2
−
𝑣
0
e
2
	​

=v
2
	​

−v
0
	​

ℎ
=
𝑑
×
𝑒
2
h=d×e
2
	​

𝑎
=
𝑒
1
⋅
ℎ
a=e
1
	​

⋅h

Then use the Möller-Trumbore algorithm.

This is one of the most important primitives because any mesh can be decomposed into triangles.
*/