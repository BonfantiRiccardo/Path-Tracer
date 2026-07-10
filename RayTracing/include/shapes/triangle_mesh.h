#ifndef TRIANGLE_MESH_H
#define TRIANGLE_MESH_H

#include "../bvh_node.h"
#include "2D/triangle.h"

#include <vector>

/**
 * A collection of triangles wrapped in its own internal BVH, presented to the
 * outer scene as a single `hittable`. Add all faces, then call `rebuild()` once
 * (the OBJ/glTF loaders do this). `hit()` is then a pure `const` traversal with
 * no lazy mutation, which is required because `camera::render` traverses the
 * scene from many worker threads concurrently.
 */
class triangle_mesh : public hittable {
  public:
    triangle_mesh() = default;

    explicit triangle_mesh(const std::vector<shared_ptr<hittable>>& triangle_objects) {
        for (const auto& object : triangle_objects) {
            triangles.add(object);
        }
        rebuild();
    }

    void add_triangle(const point3& a, const point3& b, const point3& c, const shared_ptr<material>& mat) {
        triangles.add(make_shared<triangle>(a, b - a, c - a, mat));
    }

    void add_triangle(const shared_ptr<hittable>& triangle_object) {
        triangles.add(triangle_object);
    }

    // Build (or rebuild) the internal BVH. Must be called after add_triangle()
    // and before the mesh is rendered.
    void rebuild() {
        if (triangles.empty()) {
            acceleration.reset();
            bbox = bvh_aabb::empty;
            return;
        }

        acceleration = make_shared<bvh_node>(triangles);
        bbox = acceleration->bounding_box();
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!acceleration) {
            return false;
        }

        return acceleration->hit(r, ray_t, rec);
    }

    bvh_aabb bounding_box() const override { return bbox; }

    size_t triangle_count() const { return triangles.objects.size(); }

  private:
    hittable_list triangles;
    shared_ptr<hittable> acceleration;
    bvh_aabb bbox = bvh_aabb::empty;
};

#endif // TRIANGLE_MESH_H
