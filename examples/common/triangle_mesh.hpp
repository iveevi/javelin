#pragma once

#include <set>

#include "mesh.hpp"
#include "messaging.hpp"

#include <fmt/printf.h>

namespace jvl::core {

struct TriangleMesh : Unique {
	std::vector <float3> positions;
	std::vector <float3> normals;
	std::vector <float2> uvs;
	std::vector <int3> triangles;
	std::vector <int> materials;
	std::set <int> material_usage;

	TriangleMesh() : Unique(new_uuid <TriangleMesh> ()) {}

	static std::optional <TriangleMesh> from(const Mesh &m) {
		TriangleMesh tm;

		tm.uuid = new_uuid <TriangleMesh> ();

		auto &vprops = m.vertex_properties;
		auto &fprops = m.face_properties;

		if (auto opt_pos = vprops
			.get(Mesh::position_key)
			.as <std::vector  <float3>> ())
			tm.positions = opt_pos.value();
		else
			return std::nullopt;

		if (auto opt_normals = vprops
			.get(Mesh::normal_key)
			.as <std::vector <float3>> ())
			tm.normals = opt_normals.value();
		
		if (auto opt_uvs = vprops
			.get(Mesh::uv_key)
			.as <std::vector <float2>> ())
			tm.uvs = opt_uvs.value();

		if (auto opt_tris = fprops
			.get(Mesh::triangle_key)
			.as <std::vector <int3>> ())
			tm.triangles = opt_tris.value();

		if (auto opt_materials = fprops
			.get(Mesh::material_key)
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

				mesh.positions.push_back(float3(x, y, z));
			}
		}

		// Generate indices for sphere (simplified triangulation)
		for (int32_t lat = 0; lat < resolution; lat++) {
			for (int32_t lon = 0; lon < resolution; lon++) {
				int first = lat * resolution + lon;
				int second = first + resolution;

				int3 t0 = { first, second, first + 1 };
				int3 t1 = { second, second + 1, first + 1 };

				mesh.triangles.push_back(t0);
				mesh.triangles.push_back(t1);
			}
		}

		return mesh;
	}
};

} // namespace jvl::core
