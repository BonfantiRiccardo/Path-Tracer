# VulkanGPURT (Vulkan GPU Path Tracer)

This project runs a Vulkan compute shader without creating a window or swapchain and writes the render result to an image file (`.ppm`).

The codebase is split into small modules so it is easier to map to the CPU project structure:
- `src/main.cpp`: tiny entrypoint
- `include/cli.h` + `src/cli.cpp`: CLI parsing and help text
- `include/path_tracer_app.h` + `src/path_tracer_app.cpp`: high-level app wrapper
- `src/path_tracer_setup.cpp`: Vulkan setup, buffers, descriptors, pipeline
- `src/path_tracer_runtime.cpp`: dispatch, tonemap, image write, cleanup
- `include/scene.h`: self-contained scene/material/camera definitions
- `shaders/path_tracer.comp`: shader entrypoint
- `shaders/path_tracer_lib.glsl`: shader utility and tracing logic

## Build

```bat
build.bat
```

Output binary:
- `out/vulkan_gpu_rt.exe`

## Run

From `out` or with full path:

```bat
vulkan_gpu_rt.exe --width 1200 --height 675 --spp 20 --bounces 10 --scene weekend --output image.ppm
```

Supported arguments:
- `--width <int>`
- `--height <int>`
- `--spp <int>`
- `--bounces <int>`
- `--seed <int>`
- `--scene <weekend|two-sphere>`
- `--output <path>`

Default render parameters match the CPU main scene setup:
- Camera: `vfov=20`, `lookfrom=(13,2,3)`, `lookat=(0,0,0)`, `viewup=(0,1,0)`, `defocus_angle=0.6`, `focus_dist=10`
- Image and path tracing defaults: `width=1200`, `height=675` (16:9), `spp=20`, `max_bounces=10`


## GPU selection

The app prefers NVIDIA GPUs (`vendorID = 0x10DE`) when available and falls back to the best compute-capable device.
