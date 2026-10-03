#ifndef VULKAN_GPU_RT_SCENE_H
#define VULKAN_GPU_RT_SCENE_H

#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace vkgpu {

enum class MaterialType : uint32_t {
    Lambertian = 0,
    Metal = 1,
    Dielectric = 2,
};

struct SceneMaterial {
    MaterialType type = MaterialType::Lambertian;
    std::array<float, 3> albedo = {0.8f, 0.8f, 0.8f};
    float fuzz = 0.0f;
    float refractionIndex = 1.0f;
};

struct SceneSphere {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float radius = 0.5f;
    SceneMaterial material;
};

struct SceneCamera {
    float vfov = 20.0f;
    std::array<float, 3> lookfrom = {13.0f, 2.0f, 3.0f};
    std::array<float, 3> lookat = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> viewup = {0.0f, 1.0f, 0.0f};
    float defocusAngle = 0.6f;
    float focusDist = 10.0f;
};

struct SceneDescription {
    SceneCamera camera;
    std::vector<SceneSphere> spheres;
};

inline float randomFloat01() {
    return static_cast<float>(std::rand() / (RAND_MAX + 1.0));
}

inline float randomFloat(float minValue, float maxValue) {
    return minValue + (maxValue - minValue) * randomFloat01();
}

inline std::array<float, 3> randomColor(float minValue = 0.0f, float maxValue = 1.0f) {
    return {randomFloat(minValue, maxValue), randomFloat(minValue, maxValue), randomFloat(minValue, maxValue)};
}

inline std::array<float, 3> mulColor(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

/**
 * Creates a scene description that resembles the final scene from the "Ray Tracing in One Weekend" book.
 * The scene consists of a large ground sphere and many smaller spheres with random positions and materials.
 * There are also three larger spheres with specific materials placed in the center of the scene.
 * The camera is positioned to look at the center of the scene from a distance.
 */
inline SceneDescription makeWeekendFinalScene() {
    SceneDescription scene{};

    scene.spheres.push_back(SceneSphere{{0.0f, -1000.0f, 0.0f}, 1000.0f, SceneMaterial{MaterialType::Lambertian, {0.5f, 0.5f, 0.5f}, 0.0f, 1.0f}});

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            const float chooseMat = randomFloat01();
            const std::array<float, 3> center = {
                static_cast<float>(a) + 0.9f * randomFloat01(),
                0.2f,
                static_cast<float>(b) + 0.9f * randomFloat01(),
            };

            const float dx = center[0] - 4.0f;
            const float dy = center[1] - 0.2f;
            const float dz = center[2] - 0.0f;
            const float distSq = dx * dx + dy * dy + dz * dz;
            if (distSq <= 0.81f) {
                continue;
            }

            SceneMaterial mat{};
            if (chooseMat < 0.8f) {
                mat.type = MaterialType::Lambertian;
                mat.albedo = mulColor(randomColor(), randomColor());
                mat.fuzz = 0.0f;
                mat.refractionIndex = 1.0f;
            } else if (chooseMat < 0.95f) {
                mat.type = MaterialType::Metal;
                mat.albedo = randomColor(0.5f, 1.0f);
                mat.fuzz = randomFloat(0.0f, 0.5f);
                mat.refractionIndex = 1.0f;
            } else {
                mat.type = MaterialType::Dielectric;
                mat.albedo = {1.0f, 1.0f, 1.0f};
                mat.fuzz = 0.0f;
                mat.refractionIndex = 1.5f;
            }

            scene.spheres.push_back(SceneSphere{center, 0.2f, mat});
        }
    }

    scene.spheres.push_back(SceneSphere{{0.0f, 1.0f, 0.0f}, 1.0f, SceneMaterial{MaterialType::Dielectric, {1.0f, 1.0f, 1.0f}, 0.0f, 1.5f}});
    scene.spheres.push_back(SceneSphere{{-4.0f, 1.0f, 0.0f}, 1.0f, SceneMaterial{MaterialType::Lambertian, {0.4f, 0.2f, 0.1f}, 0.0f, 1.0f}});
    scene.spheres.push_back(SceneSphere{{4.0f, 1.0f, 0.0f}, 1.0f, SceneMaterial{MaterialType::Metal, {0.7f, 0.6f, 0.5f}, 0.0f, 1.0f}});

    return scene;
}

/**
 * Used for debugging purposes. Creates a simple scene with just two spheres.
 */
inline SceneDescription makeTwoSphereScene() {
    SceneDescription scene{};
    scene.camera.lookfrom = {13.0f, 2.0f, 3.0f};
    scene.camera.lookat = {0.0f, 0.0f, -1.0f};
    scene.spheres = {
        SceneSphere{{0.0f, -100.5f, -1.0f}, 100.0f, SceneMaterial{MaterialType::Lambertian, {0.7f, 0.7f, 0.7f}, 0.0f, 1.0f}},
        SceneSphere{{0.0f, 0.0f, -1.2f}, 0.5f, SceneMaterial{MaterialType::Lambertian, {0.8f, 0.2f, 0.2f}, 0.0f, 1.0f}},
    };
    return scene;
}

inline SceneDescription buildSceneByName(const std::string& sceneName) {
    if (sceneName == "weekend") {
        return makeWeekendFinalScene();
    }
    if (sceneName == "two-sphere") {
        return makeTwoSphereScene();
    }

    throw std::runtime_error("Unknown scene: " + sceneName + " (available: weekend, two-sphere)");
}

}  // namespace vkgpu

#endif  // VULKAN_GPU_RT_SCENE_H