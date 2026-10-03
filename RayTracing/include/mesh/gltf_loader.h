#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include "../shapes/triangle_mesh.h"
#include "../shapes/2D/textured_triangle.h"
#include "mesh_material.h"
#include "../external/cgltf.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

inline point3 transform_point(const cgltf_float* matrix, const point3& p) {
    return point3(
        matrix[0] * p.x() + matrix[4] * p.y() + matrix[8] * p.z() + matrix[12],
        matrix[1] * p.x() + matrix[5] * p.y() + matrix[9] * p.z() + matrix[13],
        matrix[2] * p.x() + matrix[6] * p.y() + matrix[10] * p.z() + matrix[14]
    );
}

// Transform a normal by the inverse-transpose of the matrix' upper-left 3x3
// (the "normal matrix"), so normals stay perpendicular to the surface even
// under non-uniform scale. The result is renormalized, so the 1/det factor of
// the true inverse-transpose is dropped (final orientation is fixed by
// set_face_normal). Column-major layout, matching cgltf.
inline vec3 transform_normal(const cgltf_float* m, const vec3& n) {
    const double a = m[0], b = m[4], c = m[8];
    const double d = m[1], e = m[5], f = m[9];
    const double g = m[2], h = m[6], i = m[10];

    const vec3 transformed(
        (e * i - f * h) * n.x() + (f * g - d * i) * n.y() + (d * h - e * g) * n.z(),
        (c * h - b * i) * n.x() + (a * i - c * g) * n.y() + (b * g - a * h) * n.z(),
        (b * f - c * e) * n.x() + (c * d - a * f) * n.y() + (a * e - b * d) * n.z()
    );

    if (dot(transformed, transformed) < 1e-16) {
        return n;
    }
    return unit_vector(transformed);
}

inline std::filesystem::path resolve_gltf_image_path(const std::filesystem::path& mesh_path, const cgltf_image* image) {
    if (!image || !image->uri) {
        return {};
    }

    std::string uri(image->uri);

    // Embedded data-URI images ("data:image/png;base64,...") are not files on
    // disk; treat them as embedded (fall back to the factor color) instead of
    // building a bogus path that would load as the cyan "missing texture".
    if (uri.rfind("data:", 0) == 0) {
        return {};
    }

    // glTF image URIs are percent-encoded (e.g. spaces as %20); decode in place.
    if (!uri.empty()) {
        const cgltf_size decoded_length = cgltf_decode_uri(&uri[0]);
        uri.resize(decoded_length);
    }

    std::filesystem::path image_path(uri);
    if (image_path.is_relative()) {
        image_path = mesh_path.parent_path() / image_path;
    }

    return image_path.lexically_normal();
}

// Build an image_texture from a glTF texture view, or return nullptr when the
// image is absent or embedded (data-URI / .glb). image_texture can only read
// from disk, so embedded images fall back to the corresponding factor color.
inline shared_ptr<texture> gltf_image_texture(const std::filesystem::path& mesh_path, const cgltf_texture_view& view) {
    if (!view.texture || !view.texture->image) {
        return nullptr;
    }

    const auto texture_path = resolve_gltf_image_path(mesh_path, view.texture->image);
    if (texture_path.empty()) {
        std::cerr << "glTF image is embedded or has no URI; using factor color instead in "
                  << mesh_path.string() << '\n';
        return nullptr;
    }

    const auto texture_file = texture_path.string();
    return make_shared<image_texture>(texture_file.c_str());
}

inline shared_ptr<material> gltf_material_for_primitive(const std::filesystem::path& mesh_path, const cgltf_material* gltf_material) {
    if (!gltf_material) {
        return make_shared<lambertian>(color(0.75, 0.75, 0.75));
    }

    const auto& pbr = gltf_material->pbr_metallic_roughness;

    // Base color: texture if present, otherwise the constant factor.
    shared_ptr<texture> base_tex = gltf_image_texture(mesh_path, pbr.base_color_texture);
    if (!base_tex) {
        base_tex = make_shared<solid_color>(color(
            pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]));
    }

    // Emissive: additive on top of the base shading. Per glTF the emitted color
    // is emissiveTexture * emissiveFactor * emissiveStrength, so fold the factor
    // and strength into the texture (or use them as a solid color when there is
    // no texture). A zero factor means no emission even if a texture is present.
    const double emissive_strength = gltf_material->has_emissive_strength
        ? gltf_material->emissive_strength.emissive_strength : 1.0;
    const color emissive_scale(
        gltf_material->emissive_factor[0] * emissive_strength,
        gltf_material->emissive_factor[1] * emissive_strength,
        gltf_material->emissive_factor[2] * emissive_strength);

    if (emissive_scale.x() > 0.0 || emissive_scale.y() > 0.0 || emissive_scale.z() > 0.0) {
        shared_ptr<texture> emissive_map = gltf_image_texture(mesh_path, gltf_material->emissive_texture);
        shared_ptr<texture> emissive_tex;
        if (emissive_map) {
            emissive_tex = make_shared<scaled_texture>(emissive_map, emissive_scale);
        } else {
            emissive_tex = make_shared<solid_color>(emissive_scale);
        }
        return make_shared<lambertian_emissive>(base_tex, emissive_tex);
    }

    // Metallic surfaces map to the existing `metal` material. This uses a single
    // tint (base_color_factor) and a scalar roughness -> fuzz, NOT the per-texel
    // metallic-roughness map; a faithful mapping needs a microfacet BRDF.
    if (pbr.metallic_factor > 0.5) {
        const color tint(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
        const double fuzz = pbr.roughness_factor * pbr.roughness_factor;
        return make_shared<metal>(tint, fuzz);
    }

    return make_shared<lambertian>(base_tex);
}

inline void append_gltf_mesh_node(
    const cgltf_data* data,
    const cgltf_node* node,
    const std::filesystem::path& file_path,
    triangle_mesh& mesh,
    std::unordered_map<std::size_t, shared_ptr<material>>& material_cache
) {
    if (!node) {
        return;
    }

    cgltf_float world_matrix[16] = {};
    cgltf_node_transform_world(node, world_matrix);

    if (node->mesh) {
        const cgltf_mesh* gltf_mesh = node->mesh;

        for (cgltf_size primitive_index = 0; primitive_index < gltf_mesh->primitives_count; ++primitive_index) {
            const cgltf_primitive& primitive = gltf_mesh->primitives[primitive_index];
            if (primitive.type != cgltf_primitive_type_triangles) {
                continue;
            }

            const cgltf_accessor* position_accessor = nullptr;
            const cgltf_accessor* texcoord_accessor = nullptr;
            const cgltf_accessor* normal_accessor = nullptr;

            for (cgltf_size attribute_index = 0; attribute_index < primitive.attributes_count; ++attribute_index) {
                const cgltf_attribute& attribute = primitive.attributes[attribute_index];
                if (attribute.type == cgltf_attribute_type_position) {
                    position_accessor = attribute.data;
                } else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) {
                    // Use TEXCOORD_0 only (the set baseColor/emissive reference here);
                    // extra UV sets such as a TEXCOORD_1 lightmap are ignored.
                    texcoord_accessor = attribute.data;
                } else if (attribute.type == cgltf_attribute_type_normal) {
                    normal_accessor = attribute.data;
                }
            }

            if (!position_accessor) {
                continue;
            }

            shared_ptr<material> primitive_material = make_shared<lambertian>(color(0.75, 0.75, 0.75));
            if (primitive.material) {
                const std::size_t material_index = static_cast<std::size_t>(cgltf_material_index(data, primitive.material));
                const auto cached = material_cache.find(material_index);
                if (cached != material_cache.end()) {
                    primitive_material = cached->second;
                } else {
                    primitive_material = gltf_material_for_primitive(file_path, primitive.material);
                    material_cache.emplace(material_index, primitive_material);
                }
            }

            const auto read_position = [position_accessor, world_matrix](cgltf_size index) {
                float values[3] = {0.0f, 0.0f, 0.0f};
                cgltf_accessor_read_float(position_accessor, index, values, 3);
                return transform_point(world_matrix, point3(values[0], values[1], values[2]));
            };

            const auto read_uv = [texcoord_accessor](cgltf_size index) {
                if (!texcoord_accessor) {
                    return vec3(0.0, 0.0, 0.0);
                }

                float values[2] = {0.0f, 0.0f};
                cgltf_accessor_read_float(texcoord_accessor, index, values, 2);
                // glTF UVs use a top-left origin; image_texture expects a
                // bottom-left origin (v up), so flip V here.
                return vec3(values[0], 1.0 - values[1], 0.0);
            };

            const auto read_normal = [normal_accessor, world_matrix](cgltf_size index) {
                float values[3] = {0.0f, 0.0f, 0.0f};
                cgltf_accessor_read_float(normal_accessor, index, values, 3);
                return transform_normal(world_matrix, vec3(values[0], values[1], values[2]));
            };

            const auto add_face = [&](cgltf_size i0, cgltf_size i1, cgltf_size i2) {
                if (normal_accessor) {
                    mesh.add_triangle(make_shared<textured_triangle>(
                        read_position(i0), read_position(i1), read_position(i2),
                        read_uv(i0), read_uv(i1), read_uv(i2),
                        read_normal(i0), read_normal(i1), read_normal(i2),
                        primitive_material
                    ));
                } else {
                    mesh.add_triangle(make_shared<textured_triangle>(
                        read_position(i0), read_position(i1), read_position(i2),
                        read_uv(i0), read_uv(i1), read_uv(i2),
                        primitive_material
                    ));
                }
            };

            if (primitive.indices != nullptr) {
                if (primitive.indices->count % 3 != 0) {
                    std::cerr << "glTF primitive has a non-triangular index count in " << file_path.string() << '\n';
                    continue;
                }

                for (cgltf_size triangle_index = 0; triangle_index < primitive.indices->count; triangle_index += 3) {
                    add_face(
                        cgltf_accessor_read_index(primitive.indices, triangle_index + 0),
                        cgltf_accessor_read_index(primitive.indices, triangle_index + 1),
                        cgltf_accessor_read_index(primitive.indices, triangle_index + 2)
                    );
                }
            } else {
                if (position_accessor->count % 3 != 0) {
                    std::cerr << "glTF position accessor does not contain a multiple of three vertices in " << file_path.string() << '\n';
                    continue;
                }

                for (cgltf_size triangle_index = 0; triangle_index < position_accessor->count; triangle_index += 3) {
                    add_face(triangle_index + 0, triangle_index + 1, triangle_index + 2);
                }
            }
        }
    }

    for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index) {
        append_gltf_mesh_node(data, node->children[child_index], file_path, mesh, material_cache);
    }
}

inline shared_ptr<triangle_mesh> load_gltf_mesh(const std::filesystem::path& file_path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    const std::string file_name = file_path.string();
    cgltf_result result = cgltf_parse_file(&options, file_name.c_str(), &data);
    if (result != cgltf_result_success) {
        std::cerr << "glTF parse failed for " << file_path.string() << "\n";
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, file_name.c_str());
    if (result != cgltf_result_success) {
        std::cerr << "glTF buffer load failed for " << file_path.string() << "\n";
        cgltf_free(data);
        return nullptr;
    }

    auto mesh = make_shared<triangle_mesh>();
    std::unordered_map<std::size_t, shared_ptr<material>> material_cache;

    const cgltf_scene* scene = data->scene ? data->scene : (data->scenes_count > 0 ? &data->scenes[0] : nullptr);
    if (scene) {
        for (cgltf_size node_index = 0; node_index < scene->nodes_count; ++node_index) {
            append_gltf_mesh_node(data, scene->nodes[node_index], file_path, *mesh, material_cache);
        }
    }

    mesh->rebuild();
    cgltf_free(data);
    return mesh;
}

#endif // GLTF_LOADER_H
