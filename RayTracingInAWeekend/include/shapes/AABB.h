#ifndef AABB_H
#define AABB_H

#include "../raytracing.h"
#include "../hittable.h"

/**
 * A AABB (Axis-Aligned Bounding Box) class that inherits from the hittable interface. It represents a 
 * box in 3D space defined by two corner points (min and max) and implements the hit function to 
 * determine if a ray intersects with it.
 */
class aabb : public hittable {
  public:
    aabb(const point3& min, const point3& max, shared_ptr<material> mat) : min(min), max(max), mat(mat) {}

    /**
     * AABB equation is: min.x <= P.x <= max.x, min.y <= P.y <= max.y, min.z <= P.z <= max.z, where P is a point on the box, 
     * min is the minimum corner of the box, and max is the maximum corner of the box.
     * To find the intersection of a ray with the box, we substitute the ray equation P(t) = A + t*B into the box equation,
     * which can be expressed in terms of the components of the ray and the box corners:
     * 
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        double epsilon = 1e-8; // Small threshold to handle numerical precision issues
        if (
                std::fabs(r.direction().x()) < epsilon && 
                (r.origin().x() < min.x() || r.origin().x() > max.x())  ||
                std::fabs(r.direction().y()) < epsilon && 
                (r.origin().y() < min.y() || r.origin().y() > max.y())  ||
                std::fabs(r.direction().z()) < epsilon &&
                (r.origin().z() < min.z() || r.origin().z() > max.z())
            )
            return false;       // Ray parallel to one of the axes and outside of box bounds, no hit

        // Compute intersection t values for each pair of planes (x, y, z)
        double tx1 = -infinity, tx2 = infinity;
        double ty1 = -infinity, ty2 = infinity;
        double tz1 = -infinity, tz2 = infinity;

        if (std::fabs(r.direction().x()) >= epsilon) { // Avoid /0 for rays parallel to the yz-plane
            tx1 = (min.x() - r.origin().x()) / r.direction().x();
            tx2 = (max.x() - r.origin().x()) / r.direction().x();
        }

        if (std::fabs(r.direction().y()) >= epsilon) {  // Avoid /0 for rays parallel to the xz-plane
            ty1 = (min.y() - r.origin().y()) / r.direction().y();
            ty2 = (max.y() - r.origin().y()) / r.direction().y();
        }

        if (std::fabs(r.direction().z()) >= epsilon) {  // Avoid /0 for rays parallel to the xy-plane
            tz1 = (min.z() - r.origin().z()) / r.direction().z();
            tz2 = (max.z() - r.origin().z()) / r.direction().z();
        }

        // Compute intersection t values for the slabs and find the largest tmin and smallest tmax
        double tmin = std::max(std::max(std::min(tx1, tx2), std::min(ty1, ty2)), std::min(tz1, tz2));
        double tmax = std::min(std::min(std::max(tx1, tx2), std::max(ty1, ty2)), std::max(tz1, tz2));

        if (tmax < tmin)
            return false;

        if (ray_t.contains(tmin)) rec.t = tmin;
        else if (ray_t.contains(tmax)) rec.t = tmax;
        else return false; // No valid intersection within ray_t interval

        rec.p = r.at(rec.t);

        vec3 outward_normal;
        // Determine normal based on which face was hit
        if (std::fabs(rec.p.x() - min.x()) < epsilon)       outward_normal = vec3(-1, 0, 0); // Left face
        else if (std::fabs(rec.p.x() - max.x()) < epsilon)  outward_normal = vec3(1, 0, 0); // Right face
        else if (std::fabs(rec.p.y() - min.y()) < epsilon)  outward_normal = vec3(0, -1, 0); // Bottom face
        else if (std::fabs(rec.p.y() - max.y()) < epsilon)  outward_normal = vec3(0, 1, 0); // Top face
        else if (std::fabs(rec.p.z() - min.z()) < epsilon)  outward_normal = vec3(0, 0, -1); // Back face
        else outward_normal = vec3(0, 0, 1); // Front face

        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

    shared_ptr<material> get_material() const override { return mat; }
    double area() const override { return 0.0; }
    bool sample_surface(point3 &p, vec3 &n, double &pdf) const override { return false; }

  private:
    point3 min;
    point3 max;
    shared_ptr<material> mat;
};

#endif
