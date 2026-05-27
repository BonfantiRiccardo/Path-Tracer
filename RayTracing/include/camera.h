#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"
#include "hittable_list.h"

#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

class camera
{
public:
    // Define the aspect ratio and image dimensions.
    double aspect_ratio      = 1.0;     // Ratio of image width over height
    int    image_width       = 100;     // Rendered image width in pixel count
    int    samples_per_pixel = 10;      // Count of random samples for each pixel
    int    max_depth         = 10;      // Maximum number of ray bounces into scene
    color  background;                  // Scene background color

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

        // Create a framebuffer to hold the pixel colors
        std::vector<color> framebuffer(
            static_cast<size_t>(image_width) * static_cast<size_t>(image_height)
        );

        // Determine the number of hardware threads available
        const unsigned hw_threads = std::thread::hardware_concurrency();
        const unsigned num_threads = (hw_threads == 0) ? 1u : hw_threads;
        std::atomic<int> next_row{0};       // Atomic counter to assign rows to threads
        std::atomic<int> rows_done{0};     // Atomic counter to track completed rows for progress reporting

        // Lambda function for worker threads to render assigned rows of the image
        auto worker = [&]() {
            while (true) {  // Loop until all rows are processed
                const int j = next_row.fetch_add(1, std::memory_order_relaxed);  // Get the next row index
                if (j >= image_height)
                    break;  // No more rows to process

                // Render the assigned row of pixels (same code as original single-threaded loop)
                for (int i = 0; i < image_width; i++) {
                    color pixel_color(0,0,0);

                    // Implement stratified sampling by taking sqrt_spp samples in a grid pattern within the pixel area (part of "THE REST OF YOUR LIFE" article) 
                    for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                      for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                        ray r = get_ray(i, j, s_i, s_j);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                }
                    // Update the framebuffer with the computed pixel color, applying samples scale factor
                    framebuffer[
                        static_cast<size_t>(j) * static_cast<size_t>(image_width) + static_cast<size_t>(i)
                    ] = pixel_samples_scale * pixel_color;
                }

                // Update progress after completing the row
                rows_done.fetch_add(1, std::memory_order_relaxed);
            }
        };

        // Lambda function to log rendering progress to the console
        auto progress_logger = [&]() {
            int last_done = -1;     // Init counter to track last logged progress
            while (true) {
                // Read the current atomic count of completed rows
                const int done = rows_done.load(std::memory_order_relaxed);

                if (done != last_done) {
                    // Log progress to console (overwrite previous line)
                    std::clog << "\rScanlines remaining: " << (image_height - done) << ' ' << std::flush;
                    last_done = done;
                }

                // Exit the loop when all rows are done
                if (done >= image_height)
                    break;

                // Sleep briefly to avoid excessive CPU usage while waiting for progress updates
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        };

        // Start the progress logger thread
        std::thread progress_thread(progress_logger);


        // Create worker threads vector and launch the worker function we just defined in each thread
        std::vector<std::thread> workers;
        workers.reserve(num_threads);
        for (unsigned t = 0; t < num_threads; t++) {
            workers.emplace_back(worker);  // Start worker threads, emplace_back creates (in-place) a new element at the end of the vector
        }
        for (auto& w : workers) {
            w.join();  // Wait for all worker threads to finish
        }

        progress_thread.join();  // Wait for the progress logger thread to finish

        std::clog << "\rAll scanlines completed. Writing output file... \n" << std::flush;

        // When each thread is done, open the output file for writing
        std::ofstream out_file(output_file_path);
        if (!out_file) {
            std::cerr << "Could not open output file: " << output_file_path << "\n";
            return;
        }

        // Final render loop
        out_file << "P3\n" << image_width << " " << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                // Simply write the color of the framebuffer to the file
                write_color(out_file, 
                            framebuffer[static_cast<size_t>(j) * static_cast<size_t>(image_width) + static_cast<size_t>(i)]
                        );
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

    // Stratified sampling parameters
    int    sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp

    void initialize() {
        // Calculate the image height, and ensure that it's at least 1.
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // Calculate the square root of the number of samples per pixel, and its reciprocal (STRATIFIED SAMPLING)
        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

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
     * Returns a ray originating from the defocus disk and directed at a randomly sampled point around the pixel location i, j for stratified sample square s_i, s_j
     */
    ray get_ray(int i, int j, int s_i, int s_j) const {
        auto offset = sample_square_stratified(s_i, s_j);       // Get a random offset within the pixel area for stratified sampling

        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);
    }

    /**
     * Returns the vector to a random point in the square sub-pixel specified by grid
     * indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].
     */
    vec3 sample_square_stratified(int s_i, int s_j) const {

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }

    /** 
     * Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    */
    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    // Returns a random point in the camera defocus disk.
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
        
        // If the ray hits nothing, return the background color.
        if (!world.hit(r, interval(0.001, infinity), rec))
            return background;

        ray scattered;
        color attenuation;
        color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered))
            return color_from_emission;

        color color_from_scatter = attenuation * ray_color(scattered, depth-1, world);

        return color_from_emission + color_from_scatter;

    }
};

#endif
