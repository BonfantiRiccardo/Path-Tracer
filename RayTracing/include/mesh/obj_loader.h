#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "../shapes/triangle_mesh.h"
#include "../shapes/2D/textured_triangle.h"
#include "mesh_material.h"
#include "../external/tiny_obj_loader.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

inline std::string lower_case(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    return text;
}

inline std::filesystem::path resolve_relative_mesh_path(const std::filesystem::path& base_dir, const std::string& relative_path) {
    if (relative_path.empty()) {
        return {};
    }

    std::filesystem::path path(relative_path);
    if (path.is_relative()) {
        path = base_dir / path;
    }

    return path.lexically_normal();
}

inline shared_ptr<material> obj_material_from(const std::filesystem::path& mesh_dir, const tinyobj::material_t& obj_material) {
    // Diffuse base: texture if present, otherwise the constant Kd color.
    shared_ptr<texture> base_tex;
    if (!obj_material.diffuse_texname.empty()) {
        const auto texture_path = resolve_relative_mesh_path(mesh_dir, obj_material.diffuse_texname);
        const auto texture_file = texture_path.string();
        base_tex = make_shared<image_texture>(texture_file.c_str());
    } else {
        base_tex = make_shared<solid_color>(color(
            obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2]));
    }

    // Emissive (Ke): additive on top of the diffuse base.
    const bool has_emissive =
        obj_material.emission[0] > 0.0 ||
        obj_material.emission[1] > 0.0 ||
        obj_material.emission[2] > 0.0;
    if (has_emissive) {
        auto emissive_tex = make_shared<solid_color>(color(
            obj_material.emission[0], obj_material.emission[1], obj_material.emission[2]));
        return make_shared<lambertian_emissive>(base_tex, emissive_tex);
    }

    return make_shared<lambertian>(base_tex);
}

inline point3 obj_position(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) {
    return point3(
        attrib.vertices[3 * index.vertex_index + 0],
        attrib.vertices[3 * index.vertex_index + 1],
        attrib.vertices[3 * index.vertex_index + 2]
    );
}

inline vec3 obj_uv(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) {
    if (index.texcoord_index < 0) {
        return vec3(0.0, 0.0, 0.0);
    }

    // OBJ/MTL UVs already use a bottom-left origin (v up), matching what
    // image_texture expects, so no V flip is needed here.
    return vec3(
        attrib.texcoords[2 * index.texcoord_index + 0],
        attrib.texcoords[2 * index.texcoord_index + 1],
        0.0
    );
}

inline vec3 obj_normal(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) {
    return vec3(
        attrib.normals[3 * index.normal_index + 0],
        attrib.normals[3 * index.normal_index + 1],
        attrib.normals[3 * index.normal_index + 2]
    );
}

inline shared_ptr<triangle_mesh> load_obj_mesh(const std::filesystem::path& file_path) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.mtl_search_path = file_path.parent_path().string();

    if (!reader.ParseFromFile(file_path.string(), config)) {
        std::cerr << "OBJ load failed for " << file_path.string() << ": " << reader.Error() << '\n';
        return nullptr;
    }

    if (!reader.Warning().empty()) {
        std::cerr << "OBJ load warning for " << file_path.string() << ": " << reader.Warning() << '\n';
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    // Build each material once up front. Previously this happened per face,
    // which re-loaded every texture image from disk once for every triangle.
    std::vector<shared_ptr<material>> material_cache(materials.size());
    for (size_t material_index = 0; material_index < materials.size(); ++material_index) {
        material_cache[material_index] = obj_material_from(file_path.parent_path(), materials[material_index]);
    }
    const auto default_material = make_shared<lambertian>(color(0.75, 0.75, 0.75));

    auto mesh = make_shared<triangle_mesh>();

    for (const auto& shape : shapes) {
        size_t index_offset = 0;

        for (size_t face_index = 0; face_index < shape.mesh.num_face_vertices.size(); ++face_index) {
            const int face_vertex_count = shape.mesh.num_face_vertices[face_index];
            if (face_vertex_count < 3) {
                index_offset += static_cast<size_t>(face_vertex_count);
                continue;
            }

            const int face_material_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[face_index];
            const auto& face_material =
                (face_material_id >= 0 && face_material_id < static_cast<int>(material_cache.size()))
                    ? material_cache[static_cast<size_t>(face_material_id)]
                    : default_material;

            const auto& first = shape.mesh.indices[index_offset];

            // Fan-triangulate the (already triangulated) face.
            for (int vertex_offset = 1; vertex_offset + 1 < face_vertex_count; ++vertex_offset) {
                const auto& second = shape.mesh.indices[index_offset + static_cast<size_t>(vertex_offset)];
                const auto& third = shape.mesh.indices[index_offset + static_cast<size_t>(vertex_offset + 1)];

                const point3 v0 = obj_position(attrib, first);
                const point3 v1 = obj_position(attrib, second);
                const point3 v2 = obj_position(attrib, third);

                const vec3 uv0 = obj_uv(attrib, first);
                const vec3 uv1 = obj_uv(attrib, second);
                const vec3 uv2 = obj_uv(attrib, third);

                if (first.normal_index >= 0 && second.normal_index >= 0 && third.normal_index >= 0) {
                    mesh->add_triangle(make_shared<textured_triangle>(
                        v0, v1, v2, uv0, uv1, uv2,
                        obj_normal(attrib, first), obj_normal(attrib, second), obj_normal(attrib, third),
                        face_material));
                } else {
                    mesh->add_triangle(make_shared<textured_triangle>(v0, v1, v2, uv0, uv1, uv2, face_material));
                }
            }

            index_offset += static_cast<size_t>(face_vertex_count);
        }
    }

    mesh->rebuild();
    return mesh;
}

#endif // OBJ_LOADER_H
