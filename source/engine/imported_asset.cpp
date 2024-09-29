#include <cstddef>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fmt/printf.h>
#include <fmt/std.h>

#include "core/texture.hpp"
#include "engine/imported_asset.hpp"

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

namespace jvl::engine {

std::optional <ImportedAsset> ImportedAsset::from(const std::filesystem::path &path)
{
	using core::Mesh;
	using core::Material;

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

	if (!reader.Warning().empty())
		fmt::println("tinyobj-warning: {}", reader.Warning());

	auto &attrib = reader.GetAttrib();
	auto &shapes = reader.GetShapes();

	for (size_t s = 0; s < shapes.size(); s++) {
		buffer <float3> positions;
		buffer <float3> normals;
		buffer <float2> uvs;

		buffer <int3> triangles;
		buffer <int4> quadrilaterals;
		buffer <int> materials;

		std::unordered_map <float3, int32_t> position_map;
		std::unordered_map <tinyobj::index_t, int32_t> index_map;

		const tinyobj::shape_t &shape = shapes[s];
		
		imported_asset.names.push_back(shape.name);

		// Loop over faces (polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shape.mesh.num_face_vertices[f]);

			// Triangles or quadrilaterals
			int32_t indices[4] { -1, -1, -1, -1 };

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				if (index_map.count(idx)) {
					indices[v] = index_map[idx];
					continue;
				}

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                                float3 p { vx, vy, vz };

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
					normals.push_back({nx, ny, nz});
				}

				// Check if `texcoord_index` is zero or
				// positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					uvs.push_back({tx, ty});
				}
			}

			if (indices[3] == -1) {
				int3 f { indices[0], indices[1], indices[2] };
				triangles.push_back(f);
			} else {
				int4 f { indices[0], indices[1], indices[2], indices[3]};
				quadrilaterals.push_back(f);
			}

			materials.push_back(shape.mesh.material_ids[f]);

			index_offset += fv;
		}

		// Compose the final mesh
		Mesh mesh;

		mesh.vertex_count = positions.size();

		if (positions.size())
			mesh.vertex_properties[Mesh::position_key] = positions;
		if (normals.size())
			mesh.vertex_properties[Mesh::normal_key] = normals;
		if (uvs.size())
			mesh.vertex_properties[Mesh::uv_key] = uvs;

		if (triangles.size())
			mesh.face_properties[Mesh::triangle_key] = triangles;
		if (quadrilaterals.size())
			mesh.face_properties[Mesh::quadrilateral_key] = quadrilaterals;
		if (materials.size())
			mesh.face_properties[Mesh::material_key] = materials;

		imported_asset.geometries.push_back(mesh);
	}

	auto to_float3 = [](const tinyobj::real_t *const values) {
		return float3(values[0], values[1], values[2]);
	};

	auto &materials = reader.GetMaterials();
	for (auto &material : materials) {
		Material m;
		// TODO: check for textures
		m.values[Material::brdf_key] = "default";

		// Diffuse value
		if (material.diffuse_texname.empty())
			m.values[Material::diffuse_key] = to_float3(material.diffuse);
		else {
			auto fixed = material.diffuse_texname;
			std::replace(fixed.begin(), fixed.end(), '\\', '/');

			auto texture_path = path.parent_path() / fixed;
			core::Texture::from(texture_path);
			m.values[Material::diffuse_key] = material.diffuse_texname;
		}

		// Specular value
		if (material.specular_texname.empty())
			m.values[Material::specular_key] = to_float3(material.specular);
		else
			m.values[Material::specular_key] = material.specular_texname;

		// Ambient value
		if (material.ambient_texname.empty())
			m.values[Material::ambient_key] = to_float3(material.ambient);
		else
			m.values[Material::ambient_key] = material.ambient_texname;

		// Roughness value
		if (material.roughness_texname.empty())
			m.values[Material::roughness_key] = material.roughness;
		else
			m.values[Material::roughness_key] = material.roughness_texname;

		// Emission should only be added if it is non-zero
		float3 emission = to_float3(material.emission);
		if (length(emission) > 0)
			m.values[Material::emission_key] = emission;

		imported_asset.materials.push_back(m);
	}

	return imported_asset;
}

} // namespace jvl::core
