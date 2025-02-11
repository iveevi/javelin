#pragma once

#include <set>

#include "imported_mesh.hpp"
#include "messaging.hpp"

#include <fmt/printf.h>

namespace jvl::core {

struct TriangleMesh : Unique {
	std::vector <glm::vec3> positions;
	std::vector <glm::vec3> normals;
	std::vector <glm::vec2> uvs;
	std::vector <glm::ivec3> triangles;
	std::vector <int> materials;
	std::set <int> material_usage;

	TriangleMesh() : Unique(new_uuid <TriangleMesh> ()) {}

	static std::optional <TriangleMesh> from(const ImportedMesh &m) {
		TriangleMesh tm;

		tm.uuid = new_uuid <TriangleMesh> ();

		auto &vprops = m.vertex_properties;
		auto &fprops = m.face_properties;

		if (auto opt_pos = vprops
			.get(ImportedMesh::position_key)
			.as <std::vector  <glm::vec3>> ())
			tm.positions = opt_pos.value();
		else
			return std::nullopt;

		if (auto opt_normals = vprops
			.get(ImportedMesh::normal_key)
			.as <std::vector <glm::vec3>> ())
			tm.normals = opt_normals.value();
		
		if (auto opt_uvs = vprops
			.get(ImportedMesh::uv_key)
			.as <std::vector <glm::vec2>> ())
			tm.uvs = opt_uvs.value();

		if (auto opt_tris = fprops
			.get(ImportedMesh::triangle_key)
			.as <std::vector <glm::ivec3>> ())
			tm.triangles = opt_tris.value();

		if (auto opt_materials = fprops
			.get(ImportedMesh::material_key)
			.as <std::vector <int>> ()) {
			tm.materials = opt_materials.value();
			for (auto i : tm.materials)
				tm.material_usage.insert(i);
		}

		if (tm.triangles.empty())
			return std::nullopt;

		return tm;
	}

	static TriangleMesh uv_sphere(int32_t resolution, float radius) {
		TriangleMesh mesh;

		// Generate sphere vertices and indices (simplified version)
		for (int32_t lat = 0; lat <= resolution; lat++) {
			float theta = lat * M_PI / resolution;
			for (int32_t lon = 0; lon < resolution; lon++) {
				float phi = lon * 2 * M_PI / resolution;

				float x = radius * std::sin(theta) * std::cos(phi);
				float y = radius * std::cos(theta);
				float z = radius * std::sin(theta) * std::sin(phi);

				mesh.positions.push_back(glm::vec3(x, y, z));
			}
		}

		// Generate indices for sphere (simplified triangulation)
		for (int32_t lat = 0; lat < resolution; lat++) {
			for (int32_t lon = 0; lon < resolution; lon++) {
				int first = lat * resolution + lon;
				int second = first + resolution;

				glm::ivec3 t0 = { first, second, first + 1 };
				glm::ivec3 t1 = { second, second + 1, first + 1 };

				mesh.triangles.push_back(t0);
				mesh.triangles.push_back(t1);
			}
		}

		return mesh;
	}
};

} // namespace jvl::core
