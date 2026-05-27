#ifndef RAY_H
#define RAY_H

/**
 * A class representing a ray in 3D space.
 */
class ray {
    public:
        ray() {}

        // Constructor to initialize the ray with an origin point in space and a (unit) direction.
        ray(const point3& origin, const vec3& direction, double time): orig(origin), dir(direction), tm(time) {}

        ray(const point3& origin, const vec3& direction): ray(origin, direction, 0) {} // Overloaded constructor with default time = 0

        // Accessor functions to retrieve the origin and direction of the ray. They return immutable references to the internal data members.
        point3 origin() const { return orig; }
        vec3 direction() const { return dir; }

        double time() const { return tm; }

        // Compute the point along the ray at parameter t. Given by formula: P(t) = A + t*B, where A is the origin and B is the direction.
        point3 at(double t) const {
            return orig + t*dir;
        }

    private:
        point3 orig; // The starting point of the ray in 3D space.
        vec3 dir;    // The direction of the ray, typically a unit vector.
        double tm;   // Time parameter for motion blur


};

#endif