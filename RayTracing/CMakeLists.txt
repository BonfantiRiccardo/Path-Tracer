cmake_minimum_required(VERSION 3.21)

project(ray_tracing_cpu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(raytracing_headers INTERFACE)
target_include_directories(raytracing_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(raytracing_add_executable target_name source_file)
    add_executable(${target_name} ${source_file})
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

raytracing_add_executable(main src/main.cpp)
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