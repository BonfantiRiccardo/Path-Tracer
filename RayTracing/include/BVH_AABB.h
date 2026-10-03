#ifndef BVH_AABB_H
#define BVH_AABB_H

#include "raytracing.h"

/**
 * A BVH_AABB class that represents an axis-aligned bounding box used in a Bounding Volume Hierarchy (BVH) for efficient ray-object intersection tests.
 * It contains three intervals representing the bounds along the x, y, and z axes, and implements a hit function to determine if a ray intersects with the bounding box.
 */
class bvh_aabb {
public:
    interval x, y, z;

    bvh_aabb() {} // The default BVH_AABB is empty, since intervals are empty by default.

    bvh_aabb(const interval& x, const interval& y, const interval& z)
        : x(x), y(y), z(z) {
        pad_to_minimums();
    }

    bvh_aabb(const point3& a, const point3& b) {
        // Treat the two points a and b as extrema for the bounding box, so we don't require a
        // particular minimum/maximum coordinate order.

        x = (a[0] <= b[0]) ? interval(a[0], b[0]) : interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? interval(a[1], b[1]) : interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? interval(a[2], b[2]) : interval(b[2], a[2]);

        pad_to_minimums();
    }

    bvh_aabb(const bvh_aabb& box0, const bvh_aabb& box1) {
        x = interval(box0.x, box1.x);
        y = interval(box0.y, box1.y);
        z = interval(box0.z, box1.z);

        pad_to_minimums();
    }


    const interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    bool hit(const ray& r, interval ray_t) const {
        const point3& ray_orig = r.origin();
        const vec3&   ray_dir  = r.direction();

        for (int axis = 0; axis < 3; axis++) {              // Loop over x, y, z axes
            const interval& ax = axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];       // Compute the inverse of the ray direction component for this axis

            auto t0 = (ax.min - ray_orig[axis]) * adinv;    // Compute the t value where the ray intersects the near plane of the slab for this axis
            auto t1 = (ax.max - ray_orig[axis]) * adinv;    // Same for the far plane of the slab

            if (t0 < t1) {                                  // Ensure t0 is the smaller value and t1 is the larger value
                if (t0 > ray_t.min) ray_t.min = t0;         // Update the minimum t value for the ray interval to be the maximum of the current minimum and t0
                if (t1 < ray_t.max) ray_t.max = t1;
            } else {
                if (t1 > ray_t.min) ray_t.min = t1;
                if (t0 < ray_t.max) ray_t.max = t0;
            }

            if (ray_t.max <= ray_t.min)                     // If the maximum t value is less than or equal to the minimum t value, it means the ray misses the box
                return false;
        }
        return true;
    }

    int longest_axis() const {
        // Returns the index of the longest axis of the bounding box.

        if (x.size() > y.size())
            return x.size() > z.size() ? 0 : 2;
        else
            return y.size() > z.size() ? 1 : 2;
    }

    static const bvh_aabb empty, universe;

private:
    void pad_to_minimums() {
        // Adjust the AABB so that no side is narrower than some delta, padding if necessary.

        double delta = 0.0001;
        if (x.size() < delta)    x = x.expand(delta);
        if (y.size() < delta)    y = y.expand(delta);
        if (z.size() < delta)    z = z.expand(delta);
    }
};

inline const bvh_aabb bvh_aabb::empty    = bvh_aabb(interval::empty,    interval::empty,    interval::empty);
inline const bvh_aabb bvh_aabb::universe = bvh_aabb(interval::universe, interval::universe, interval::universe);

inline bvh_aabb operator+(const bvh_aabb& bbox, const vec3& offset) {
    return bvh_aabb(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z());
}

inline bvh_aabb operator+(const vec3& offset, const bvh_aabb& bbox) {
    return bbox + offset;
}

#endif // BVH_AABB_H