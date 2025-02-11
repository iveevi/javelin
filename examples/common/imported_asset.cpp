#include <cstddef>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fmt/printf.h>
#include <fmt/std.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "imported_asset.hpp"

template <>
struct std::hash <tinyobj::index_t> {
	size_t operator()(const tinyobj::index_t &k) const {
		auto h = hash <int> ();
		return ((h(k.vertex_index) ^ (h(k.normal_index) << 1)) >> 1) ^ (h(k.texcoord_index) << 1);
	}
};

namespace tinyobj {

inline bool operator==(const tinyobj::index_t &a, const tinyobj::index_t &b)
{
	return std::tie(a.vertex_index, a.normal_index, a.texcoord_index)
		== std::tie(b.vertex_index, b.normal_index, b.texcoord_index);
}

} // namespace tinyobj

std::optional <ImportedAsset> ImportedAsset::from(const std::filesystem::path &path)
{
	ImportedAsset imported_asset;
	imported_asset.path = path;

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig reader_config;

	reader_config.mtl_search_path = path.parent_path();

	if (!reader.ParseFromFile(path, reader_config)) {
		if (!reader.Error().empty())
			fmt::println("tinyobj-error: {}", reader.Error());

		return std::nullopt;
	}

	// if (!reader.Warning().empty())
	// 	fmt::println("tinyobj-warning: {}", reader.Warning());

	auto &attrib = reader.GetAttrib();
	auto &shapes = reader.GetShapes();

	std::set <int32_t> referenced_materials;

	for (size_t s = 0; s < shapes.size(); s++) {
		std::vector <glm::vec3> positions;
		std::vector <glm::vec3> normals;
		std::vector <glm::vec2> uvs;

		std::vector <glm::ivec3> triangles;
		std::vector <glm::ivec4> quadrilaterals;
		std::vector <int> materials;

		std::unordered_map <glm::vec3, int32_t> position_map;
		std::unordered_map <tinyobj::index_t, int32_t> index_map;

		const tinyobj::shape_t &shape = shapes[s];
		const tinyobj::mesh_t &smesh = shape.mesh;
		
		imported_asset.names.push_back(shape.name);

		// Loop over faces (polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < smesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(smesh.num_face_vertices[f]);

			// Triangles or quadrilaterals
			int32_t indices[4] { -1, -1, -1, -1 };

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = smesh.indices[index_offset + v];
				if (index_map.count(idx)) {
					indices[v] = index_map[idx];
					continue;
				}

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                                glm::vec3 p { vx, vy, vz };

				if (position_map.count(p)) {
					indices[v] = position_map[p];
					continue;
				} else {
					indices[v] = positions.size();
				}

				positions.push_back(p);

				// Check if `normal_index` is zero or positive.
				// negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
					normals.push_back({ nx, ny, nz });
				}

				// Check if `texcoord_index` is zero or
				// positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					uvs.push_back({ tx, ty });
				}
			}

			if (indices[3] == -1) {
				glm::ivec3 f { indices[0], indices[1], indices[2] };
				triangles.push_back(f);
			} else {
				glm::ivec4 f { indices[0], indices[1], indices[2], indices[3]};
				quadrilaterals.push_back(f);
			}

			int32_t mid = smesh.material_ids[f];

			// TODO: reference the last one by default?
			if (mid < 0)
				mid = 0;

			materials.push_back(mid);
			referenced_materials.insert(mid);

			index_offset += fv;
		}

		// Compose the final mesh
		ImportedMesh mesh;

		mesh.vertex_count = positions.size();

		if (positions.size())
			mesh.vertex_properties[ImportedMesh::position_key] = positions;
		if (normals.size())
			mesh.vertex_properties[ImportedMesh::normal_key] = normals;
		if (uvs.size())
			mesh.vertex_properties[ImportedMesh::uv_key] = uvs;

		if (triangles.size())
			mesh.face_properties[ImportedMesh::triangle_key] = triangles;
		if (quadrilaterals.size())
			mesh.face_properties[ImportedMesh::quadrilateral_key] = quadrilaterals;
		if (materials.size())
			mesh.face_properties[ImportedMesh::material_key] = materials;

		imported_asset.geometries.push_back(mesh);
	}

	auto convert_to_color3 = [](const tinyobj::real_t *const values) {
		return color3(values[0], values[1], values[2]);
	};


	auto resolved_texture = [&](const std::string &in) {
		std::string fixed = in;
		std::replace(fixed.begin(), fixed.end(), '\\', '/');
		std::string full = path.parent_path() / fixed;
		return texture(full);
	};

	auto &materials = reader.GetMaterials();
	for (auto &material : materials) {
		ImportedMaterial m(material.name);

		// TODO: check for textures
		m.values[ImportedMaterial::brdf_key] = name("Phong");

		// Diffuse value
		if (material.diffuse_texname.empty())
			m.values[ImportedMaterial::diffuse_key] = convert_to_color3(material.diffuse);
		else
			m.values[ImportedMaterial::diffuse_key] = resolved_texture(material.diffuse_texname);

		// Specular value
		if (material.specular_texname.empty())
			m.values[ImportedMaterial::specular_key] = convert_to_color3(material.specular);
		else
			m.values[ImportedMaterial::specular_key] = resolved_texture(material.specular_texname);

		// Ambient value
		if (material.ambient_texname.empty())
			m.values[ImportedMaterial::ambient_key] = convert_to_color3(material.ambient);
		else
			m.values[ImportedMaterial::ambient_key] = resolved_texture(material.ambient_texname);

		// Roughness value
		if (material.roughness_texname.empty())
			m.values[ImportedMaterial::roughness_key] = material.roughness;
		else
			m.values[ImportedMaterial::roughness_key] = resolved_texture(material.roughness_texname);

		// Emission should only be added if it is non-zero
		glm::vec3 emission = convert_to_color3(material.emission);
		if (length(emission) > 0)
			m.values[ImportedMaterial::emission_key] = emission;

		imported_asset.materials.push_back(m);
	}

	// Fill the rest with a default material
	while (imported_asset.materials.size() < referenced_materials.size()) {
		// TODO: default_phong()
		ImportedMaterial m("default");
		m.values[ImportedMaterial::brdf_key] = name("Phong");
		m.values[ImportedMaterial::diffuse_key] = glm::vec3(1, 0, 1);
		m.values[ImportedMaterial::specular_key] = glm::vec3(0, 0, 0);
		m.values[ImportedMaterial::roughness_key] = 1.0f;

		imported_asset.materials.push_back(m);
	}

	return imported_asset;
}