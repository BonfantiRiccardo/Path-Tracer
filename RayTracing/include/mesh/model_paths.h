#ifndef MODEL_PATHS_H
#define MODEL_PATHS_H

#include <cstdlib>
#include <filesystem>
#include <string>

// Asset folders (models/, images/) live under RayTracing/. The CMake build
// mirrors them next to the executable (build/out/<config>/models|images/), so
// the program resolves assets relative to the running executable rather than the
// working directory it happens to be launched from.
//
// main() calls set_models_dir(<exe_dir>/models) and set_images_dir(<exe_dir>/images)
// once at startup, before any scene is built.

inline std::filesystem::path& models_directory() {
    static std::filesystem::path directory = "models";
    return directory;
}

inline void set_models_dir(const std::filesystem::path& directory) {
    models_directory() = directory;
}

// Resolve a model file under the models/ folder. Scenes load every model
// through this so models are only ever read from the mirrored models/ folder.
inline std::filesystem::path model_path(const std::string& relative_path) {
    return (models_directory() / relative_path).lexically_normal();
}

// Point rtw_image at the mirrored images/ folder by setting the RTW_IMAGES
// environment variable it already checks first. Must be called before any
// texture is loaded (i.e. before a scene is built).
inline void set_images_dir(const std::filesystem::path& directory) {
    const std::string value = directory.string();
#ifdef _WIN32
    _putenv_s("RTW_IMAGES", value.c_str());
#else
    setenv("RTW_IMAGES", value.c_str(), 1);
#endif
}

#endif // MODEL_PATHS_H
