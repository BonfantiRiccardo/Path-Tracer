# Path Tracer C++ Implementation

![Path Tracer Render](docs/next_week.png)


This repository contains a path tracer in two versions:

1. `RayTracing`: CPU implementation following [Ray Tracing in One Weekend](https://raytracing.github.io/).
2. `VulkanGPURT`: GPU implementation using a Vulkan compute shader (headless, no window/swapchain).

Both versions write the rendered image to a `.ppm` file.

## Hardcoded scenes results
![Weekend Scene](docs/weekend.png)
- `weekend.png`: the final scene of the first book, many small spheres with diffuse, metal and glass materials.
![Room Interior Scene](docs/room_interior.png)
- `room_interior.png`: a textured room with a table, a chair, a rug, a painting, a glass bottle and a ceiling light.
![Abstract Life Scene](docs/abstract_life.png)
- `abstract_life.png`: planes, boxes, spheres, cylinders, cones and triangles with mixed materials and depth of field.
![Cornell Box Scene](docs/final_cornell.png)
- `final_cornell.png`: the classic Cornell box with a rotated box and a glass sphere.
![Ferrari Scene](docs/ferrari.png)
- `ferrari.png`: a 1987 Ferrari F40 glTF model on a street at night, with a sky dome, a brick wall and a street lamp.


## Build And Run (CPU)

Project folder: `RayTracing`

Build:

```bash
cmake -S RayTracing -B RayTracing/build
cmake --build RayTracing/build --config Release
```

Binary output:

- `RayTracing/build/out/<config>/main` or `main.exe` depending on platform and generator

The CMake build also produces the small demo executables from `RayTracing/src/`:

- `cos_cubed`
- `cos_density`
- `estimate_halfway`
- `importance_sphere`
- `integrate_x_sq`
- `pi`
- `sphere_plot`

Run (pass `--help` to see all options):

```bash
# default scene (cornell_box) at the default settings
.\RayTracing\build\out\Release\main.exe

# pick a built-in scene by its file name in include/scenes/ and override the render settings
.\RayTracing\build\out\Release\main.exe --scene ferrari_1987 --width 1200 --spp 1000 --depth 100

# render a single mesh file (path tried as-given, then relative to models/)
.\RayTracing\build\out\Release\main.exe --mesh 1987_ferrari_f40/scene.gltf
```

Options:

- `--scene <name>`: built-in scene, selected by its exact file name (default `cornell_box`). Run with `--help` for the full list.
- `--mesh <path>`: render one `.obj`/`.gltf`/`.glb`; overrides `--scene`. The path is resolved relative to the working directory (or absolute), then relative to the `models/` folder.
- `--width N`, `--spp N`, `--depth N`: image width, samples per pixel, and max ray bounces.
- `-h`, `--help`: print usage and the list of scenes.

The build copies the `models/` and `images/` asset folders next to the executable, so assets load regardless of the working directory.

Output image:

- `RayTracing/build/out/<config>/image.ppm`

Third-party header-only libraries in `RayTracing/include/external/`:

- `tiny_obj_loader.h` for OBJ files
- `cgltf.h` for glTF 2.0 files
- `stb_image.h` for image textures

`--mesh` uses `mesh_scene(...)` from `RayTracing/include/scenes/mesh.h`: it picks the loader from the extension (`.obj`, `.gltf` or `.glb`), builds a BVH over the imported triangles and places the camera so the whole model is in view.

## Build And Run (Vulkan GPU)

Project folder: `VulkanGPURT`

Requires the Vulkan SDK installed and the `VULKAN_SDK` environment variable set (CMake uses it to find `glslc` for compiling the compute shader).

Build (run from the repository root):

```bash
cmake -S VulkanGPURT -B VulkanGPURT/build
cmake --build VulkanGPURT/build --config Release
```

Binary output:

- `VulkanGPURT/build/<config>/vulkan_gpu_rt.exe` (e.g. `VulkanGPURT/build/Release/vulkan_gpu_rt.exe`)

Shader output compiled and copied next to the executable by the build:

- `VulkanGPURT/build/<config>/shaders/path_tracer.comp.spv`

Run example (launch from the repository root so the shader is found):

```bat
.\VulkanGPURT\build\Release\vulkan_gpu_rt.exe --width 1200 --height 675 --spp 1000 --bounces 100 --scene weekend --output VulkanGPURT/out/image.ppm
```

The renderer looks for `path_tracer.comp.spv` relative to the current working directory: first in `VulkanGPURT/build/Release/shaders/` and `VulkanGPURT/build/Debug/shaders/`, then in `shaders/`, `../shaders/` and `../../shaders/`. Run it from the repository root as shown above, or from a folder that contains `shaders/path_tracer.comp.spv` (for example `VulkanGPURT/build/Release/`).

Supported arguments:

- `--width <int>`
- `--height <int>`
- `--aspect <float>`
- `--spp <int>`
- `--bounces <int>`
- `--seed <int>`
- `--scene <weekend|two-sphere>`
- `--output <path>`
- `--help`

Default render configuration:

- `width=1200`, `height=675`, `aspect=16:9`
- `spp=20`, `max_bounces=10`, `seed=1`
- `scene=weekend`
- `output=render.ppm`
