cmake_minimum_required(VERSION 3.21)
project(vulkan_gpu_rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

find_program(
    GLSLC_EXECUTABLE
    NAMES glslc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/Bin32"
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK and ensure glslc is available.")
endif()

set(PATH_TRACER_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/path_tracer.comp)
set(PATH_TRACER_SHADER_LIB ${CMAKE_CURRENT_SOURCE_DIR}/shaders/path_tracer_lib.glsl)
set(PATH_TRACER_SHADER_BINARY ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracer.comp.spv)

add_custom_command(
    OUTPUT ${PATH_TRACER_SHADER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLC_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${PATH_TRACER_SHADER_SOURCE} -o ${PATH_TRACER_SHADER_BINARY}
    DEPENDS ${PATH_TRACER_SHADER_SOURCE} ${PATH_TRACER_SHADER_LIB}
    COMMENT "Compiling compute shader: path_tracer.comp"
)

add_custom_target(vulkan_gpurt_shaders ALL DEPENDS ${PATH_TRACER_SHADER_BINARY})

add_executable(
    vulkan_gpu_rt
    src/main.cpp
    src/cli.cpp
    src/path_tracer_app.cpp
    src/path_tracer_setup.cpp
    src/path_tracer_runtime.cpp
    src/shader_file_io.cpp
    ${PATH_TRACER_SHADER_SOURCE}
    ${PATH_TRACER_SHADER_LIB}
)

add_dependencies(vulkan_gpu_rt vulkan_gpurt_shaders)

target_include_directories(vulkan_gpu_rt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(vulkan_gpu_rt PRIVATE Vulkan::Vulkan)

add_custom_command(
    TARGET vulkan_gpu_rt
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:vulkan_gpu_rt>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATH_TRACER_SHADER_BINARY} $<TARGET_FILE_DIR:vulkan_gpu_rt>/shaders/path_tracer.comp.spv
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT vulkan_gpu_rt)
set_target_properties(vulkan_gpu_rt PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_gpu_rt>")
