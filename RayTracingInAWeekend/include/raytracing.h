#ifndef RAYTRACING_H
#define RAYTRACING_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <thread>
#include <functional>
#include <cstdint>

// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

// Returns a thread-local random number generator using the Mersenne Twister algorithm
// Each thread will have its own instance of the generator, seeded with  thread ID and a random device
inline std::mt19937_64& thread_local_rng() {
    // Initialize the generator (do it only first time this function is called in each thread)
    thread_local std::mt19937_64 gen([] {
            // Get current threadId, hashes to int and casts to uint64
            const auto thread_id = static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
            // Read seed from a random device (non-deterministic OS provided source of randomness) and casts to uint64
            const auto rd = static_cast<std::uint64_t>(std::random_device{}());
            // Combine thread ID and random seed using XOR to create a unique seed for each thread
            return std::mt19937_64(thread_id ^ rd);
        }()     // Call the lambda immediately to initialize the generator with the combined seed
    );
    return gen;
}

inline double random_double() {
    // Create a thread-local random number generator and distribution
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    // Returns a random real in [0,1).
    //return std::rand() / (RAND_MAX + 1.0);
    return distribution(thread_local_rng());    // Use the thread-local generator to produce a random number
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}

inline int random_int(int min, int max) {
    // Create a random number generator and distribution
    std::uniform_int_distribution<int> distribution(min, max);
    // Returns a random integer in [min,max].
    //return int(random_double(min, max+1));
    return distribution(thread_local_rng());    // Use the thread-local generator to produce a random integer
}

// Common Headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

#endif