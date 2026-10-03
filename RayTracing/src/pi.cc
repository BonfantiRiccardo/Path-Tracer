#include "../include/raytracing.h"

#include <iostream>
#include <iomanip>

int main() {
    std::cout << std::fixed << std::setprecision(12);

    int inside_circle = 0;

    // Number of random points to generate, Law of Diminishing Returns:
    // The more points we generate, the closer we get to the actual value of Pi,
    // but the improvement becomes smaller and smaller as we increase the number of points
    /*int N = 100000;

    int runs = 0;
    while (true) {
        runs++;
        auto x = random_double(-1,1);
        auto y = random_double(-1,1);
        if (x*x + y*y < 1)
            inside_circle++;

        if (runs % 100000 == 0)
            std::cout << "\rEstimate of Pi = " << (4.0 * inside_circle) / runs;
    }*/

    // Now apply Stratified Sampling to mitigate the diminishing returns problem
    // Divide the unit square into a grid and sample within each cell
    int inside_circle_stratified = 0;
    int sqrt_N = 1000;

    for (int i = 0; i < sqrt_N; i++) {
        for (int j = 0; j < sqrt_N; j++) {
            auto x = random_double(-1,1);
            auto y = random_double(-1,1);
            if (x*x + y*y < 1)
                inside_circle++;

            x = 2*((i + random_double()) / sqrt_N) - 1;
            y = 2*((j + random_double()) / sqrt_N) - 1;
            if (x*x + y*y < 1)
                inside_circle_stratified++;
        }
    }

    std::cout
        << "Regular    Estimate of Pi = "
        << (4.0 * inside_circle) / (sqrt_N*sqrt_N) << '\n'
        << "Stratified Estimate of Pi = "
        << (4.0 * inside_circle_stratified) / (sqrt_N*sqrt_N) << '\n';


    // Stratified estimate will be better (more accurate), especially for smaller sample sizes
    // We can use this in our ray tracing algorithm to stratify the locations of the sampling positions around each pixel location,
    // which helps to reduce noise and improve the quality of the rendered image without needing to increase the number of samples

}