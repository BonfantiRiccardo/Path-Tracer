#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"
#include "hittable_list.h"

#include <fstream>
#include <filesystem>
#include <vector>

class camera
{
public:
    // Define the aspect ratio and image dimensions.
    double aspect_ratio      = 1.0;     // Ratio of image width over height
    int    image_width       = 100;     // Rendered image width in pixel count
    int    samples_per_pixel = 10;      // Count of random samples for each pixel
    int    max_depth         = 10;      // Maximum number of ray bounces into scene

    double vfov = 90;  // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);   // Point camera is looking from
    point3 lookat   = point3(0,0,-1);  // Point camera is looking at
    vec3   viewup      = vec3(0,1,0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus



    void render(const hittable &world, const std::filesystem::path& output_file_path = std::filesystem::path("out") / "image.ppm") {
        initialize();

        // Ensure output directory exists and open output file
        const auto output_dir = output_file_path.parent_path();
        if (!output_dir.empty()) {
            std::filesystem::create_directories(output_dir);
        }
        std::ofstream out_file(output_file_path);
        if (!out_file)
        {
            std::cerr << "Could not open output file: " << output_file_path << "\n";
            return;
        }

        // Render loop
        out_file << "P3\n"
                 << image_width << " " << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++)
        {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++)
            {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                write_color(out_file, pixel_samples_scale * pixel_color);
            }
        }

        std::clog << "\rDone.                 \n";
        out_file.close();
    }

private:
    int image_height;   // Rendered image height
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    point3 center;      // Camera center
    point3 pixel00_loc; // Location of pixel 0, 0
    vec3 pixel_delta_u; // Offset to pixel to the right
    vec3 pixel_delta_v; // Offset to pixel below
    vec3   u, v, w;              // Camera frame basis vectors
    vec3   defocus_disk_u;       // Defocus disk horizontal radius
    vec3   defocus_disk_v;       // Defocus disk vertical radius

    void initialize() {
        // Calculate the image height, and ensure that it's at least 1.
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // Determine color scale factor for sum of pixel samples.
        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        // Determine viewport dimensions.        
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(viewup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }


    /**
     * Returns a ray originating from the defocus disk and directed at a randomly sampled point around the pixel location i, j.
     */
    ray get_ray(int i, int j) const {
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);
    }

    /** 
     * Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    */
    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    /**
     * Computes the color seen along a ray.
     */
    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);

            
        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec)) {
            // Emitted radiance from the hit material
            color emitted = rec.mat ? rec.mat->emitted() : color(0,0,0);

            // Next-Event Estimation (direct lighting) for diffuse materials
            color direct_light(0,0,0);

            // Only perform NEE for lambertian BRDFs (diffuse)
            if (rec.mat) {
                auto lam = dynamic_cast<const lambertian*>(rec.mat.get());
                if (lam) {
                    // Attempt to find emissive objects in the scene
                    const hittable_list* list = dynamic_cast<const hittable_list*>(&world);
                    if (list && !list->objects.empty()) {
                        // Collect emissive area lights
                        std::vector<shared_ptr<hittable>> lights;
                        for (const auto &obj : list->objects) {
                            auto m = obj->get_material();
                            if (!m) continue;
                            auto Le = m->emitted();
                            if (Le.x() > 0 || Le.y() > 0 || Le.z() > 0) {
                                if (obj->area() > 0.0) lights.push_back(obj);
                            }
                        }

                        if (!lights.empty()) {
                            int n = static_cast<int>(lights.size());
                            int idx = static_cast<int>(random_double(0, (double)n));
                            auto light = lights[idx];

                            point3 p_light;
                            vec3 n_light;
                            double pdf_area = 0.0;
                            if (light->sample_surface(p_light, n_light, pdf_area) && pdf_area > 0.0) {
                                vec3 to_light = p_light - rec.p;
                                double dist2 = to_light.length_squared();
                                double dist = std::sqrt(dist2);
                                vec3 wi = unit_vector(to_light);
                                double cos_theta = dot(rec.normal, wi);
                                double cos_light = dot(n_light, -wi);
                                if (cos_theta > 0 && cos_light > 0) {
                                    // Shadow ray
                                    ray shadow(rec.p + 1e-4 * wi, wi);
                                    hit_record tmp;
                                    if (!world.hit(shadow, interval(1e-4, dist - 1e-4), tmp)) {
                                        double pdf = (1.0 / double(n)) * pdf_area;
                                        if (pdf > 0) {
                                            auto Le = light->get_material()->emitted();
                                            color albedo = lam->getTextureValue(rec.u, rec.v, rec.p);
                                            color f = albedo / pi; // Lambertian BRDF
                                            color contrib = Le * f * (cos_theta * cos_light) / (dist2 * pdf);
                                            direct_light += contrib;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // If the material scatters, accrue emitted + direct lighting + attenuated recursive contribution.
            ray scattered;
            color attenuation;
            if (rec.mat && rec.mat->scatter(r, rec, attenuation, scattered)) {
                return emitted + direct_light + attenuation * ray_color(scattered, depth-1, world);
            }

            // Non-scattering material (e.g. pure light) -> return emitted radiance (no scattering)
            return emitted + direct_light;
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        // Linear interpolation between white and blue based on the y component of the ray direction.
        // This creates a gradient background.
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }
};

#endif
