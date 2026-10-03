cmake_minimum_required(VERSION 3.21)

project(ray_tracing_cpu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(raytracing_headers INTERFACE)
target_include_directories(raytracing_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(raytracing_add_executable target_name)
    # All arguments after target_name are treated as source files, so a target
    # can be built from several translation units (e.g. main + the header-only
    # library implementation TUs).
    add_executable(${target_name} ${ARGN})
    target_link_libraries(${target_name} PRIVATE raytracing_headers)

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /EHsc)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        target_link_libraries(${target_name} PRIVATE stdc++fs)
    endif()

    set_target_properties(
        ${target_name}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>"
    )
endfunction()

raytracing_add_executable(main src/main.cpp src/cgltf_impl.cpp src/tinyobj_impl.cpp src/stb_image_impl.cpp)

# Mirror the asset folders (models/, images/) next to the main executable so
# they can be loaded relative to the binary, independent of the working
# directory. copy_directory_if_different (CMake 3.26+) skips unchanged files so
# rebuilds stay fast; older CMake falls back to a plain copy.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.26")
    set(_asset_copy_cmd copy_directory_if_different)
else()
    set(_asset_copy_cmd copy_directory)
endif()
foreach(_asset_dir models images)
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E ${_asset_copy_cmd}
                "${CMAKE_CURRENT_SOURCE_DIR}/${_asset_dir}"
                "$<TARGET_FILE_DIR:main>/${_asset_dir}"
        COMMENT "Mirroring ${_asset_dir}/ next to the main executable"
        VERBATIM)
endforeach()
raytracing_add_executable(cos_cubed src/cos_cubed.cc)
raytracing_add_executable(cos_density src/cos_density.cc)
raytracing_add_executable(estimate_halfway src/estimate_halfway.cc)
raytracing_add_executable(importance_sphere src/importance_sphere.cc)
raytracing_add_executable(integrate_x_sq src/integrate_x_sq.cc)
raytracing_add_executable(pi src/pi.cc)
raytracing_add_executable(sphere_plot src/sphere_plot.cc)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
endif()