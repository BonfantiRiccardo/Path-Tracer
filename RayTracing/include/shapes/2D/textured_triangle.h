#ifndef TEXTURED_TRIANGLE_H
#define TEXTURED_TRIANGLE_H

#include "../../hittable.h"

/**
 * A triangle carrying per-vertex UVs and, optionally, per-vertex normals.
 * When vertex normals are supplied it performs smooth (Phong) shading by
 * barycentric interpolation of the normals; otherwise it falls back to the
 * geometric face normal (flat shading). Intersection uses the Moeller-Trumbore
 * algorithm. Unlike `triangle`/`planar_primitive`, this class interpolates the
 * true texture UVs instead of dumping the barycentric coordinates.
 */
class textured_triangle : public hittable {
public:
    // Flat-shaded: normal is the geometric face normal.
    textured_triangle(
        const point3& p0,
        const point3& p1,
        const point3& p2,
        const vec3& uv0,
        const vec3& uv1,
        const vec3& uv2,
        shared_ptr<material> mat
    ) : p0(p0), p1(p1), p2(p2), uv0(uv0), uv1(uv1), uv2(uv2), mat(mat), smooth(false) {
        geometric_normal = unit_vector(cross(p1 - p0, p2 - p0));
        set_bounding_box();
    }

    // Smooth-shaded: normal is the barycentric interpolation of the vertex normals.
    textured_triangle(
        const point3& p0,
        const point3& p1,
        const point3& p2,
        const vec3& uv0,
        const vec3& uv1,
        const vec3& uv2,
        const vec3& n0,
        const vec3& n1,
        const vec3& n2,
        shared_ptr<material> mat
    ) : p0(p0), p1(p1), p2(p2), uv0(uv0), uv1(uv1), uv2(uv2),
        n0(n0), n1(n1), n2(n2), mat(mat), smooth(true) {
        geometric_normal = unit_vector(cross(p1 - p0, p2 - p0));
        set_bounding_box();
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const vec3 edge1 = p1 - p0;
        const vec3 edge2 = p2 - p0;

        const vec3 pvec = cross(r.direction(), edge2);
        const double det = dot(edge1, pvec);
        if (std::fabs(det) < 1e-12) {
            return false;
        }

        const double inv_det = 1.0 / det;
        const vec3 tvec = r.origin() - p0;
        const double bary_u = dot(tvec, pvec) * inv_det;
        if (bary_u < 0.0 || bary_u > 1.0) {
            return false;
        }

        const vec3 qvec = cross(tvec, edge1);
        const double bary_v = dot(r.direction(), qvec) * inv_det;
        if (bary_v < 0.0 || bary_u + bary_v > 1.0) {
            return false;
        }

        const double t = dot(edge2, qvec) * inv_det;
        if (!ray_t.contains(t)) {
            return false;
        }

        const double bary_w = 1.0 - bary_u - bary_v;

        rec.t = t;
        rec.p = r.at(t);
        rec.mat = mat;

        const vec3 interpolated_uv = bary_w * uv0 + bary_u * uv1 + bary_v * uv2;
        rec.u = interpolated_uv.x();
        rec.v = interpolated_uv.y();

        vec3 shading_normal = geometric_normal;
        if (smooth) {
            const vec3 interpolated_normal = bary_w * n0 + bary_u * n1 + bary_v * n2;
            if (dot(interpolated_normal, interpolated_normal) > 1e-16) {
                shading_normal = unit_vector(interpolated_normal);
            }
        }
        rec.set_face_normal(r, shading_normal);
        return true;
    }

    bvh_aabb bounding_box() const override { return bbox; }

private:
    void set_bounding_box() {
        const double epsilon = 1e-4;
        bbox = bvh_aabb(
            point3(
                std::fmin(p0.x(), std::fmin(p1.x(), p2.x())) - epsilon,
                std::fmin(p0.y(), std::fmin(p1.y(), p2.y())) - epsilon,
                std::fmin(p0.z(), std::fmin(p1.z(), p2.z())) - epsilon
            ),
            point3(
                std::fmax(p0.x(), std::fmax(p1.x(), p2.x())) + epsilon,
                std::fmax(p0.y(), std::fmax(p1.y(), p2.y())) + epsilon,
                std::fmax(p0.z(), std::fmax(p1.z(), p2.z())) + epsilon
            )
        );
    }

    point3 p0;
    point3 p1;
    point3 p2;
    vec3 uv0;
    vec3 uv1;
    vec3 uv2;
    vec3 n0;
    vec3 n1;
    vec3 n2;
    vec3 geometric_normal;
    shared_ptr<material> mat;
    bool smooth;
    bvh_aabb bbox;
};

#endif // TEXTURED_TRIANGLE_H
