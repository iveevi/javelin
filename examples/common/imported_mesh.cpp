#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "imported_mesh.hpp"

ImportedMesh &ImportedMesh::deduplicate_vertices()
{
	std::unordered_map <glm::vec3, uint32_t> map;

	std::vector <glm::vec3> new_positions;
	
	auto &positions = vertex_properties[position_key]
		.as <std::vector <glm::vec3>> ();
	
	auto &triangles = face_properties[triangle_key]
		.as <std::vector <glm::ivec3>> ();
	
	auto unique_vertex = [&](const glm::vec3 &p) -> uint32_t {
		if (map.contains(p))
			return map[p];

		uint32_t idx = new_positions.size();
		new_positions.emplace_back(p);
		map[p] = idx;
		return idx;
	};

	for (size_t i = 0; i < triangles.size(); i++) {
		auto &tri = triangles[i];

		auto &p0 = positions[tri.x];
		auto &p1 = positions[tri.y];
		auto &p2 = positions[tri.z];

		tri.x = unique_vertex(p0);
		tri.y = unique_vertex(p1);
		tri.z = unique_vertex(p2);
	}

	vertex_properties[position_key] = new_positions;
	vertex_count = new_positions.size();

	return *this;
}

ImportedMesh &ImportedMesh::recompute_normals()
{
	std::vector <glm::vec3> normals(vertex_count, glm::vec3(0));

	auto &positions = vertex_properties[position_key]
		.as <std::vector <glm::vec3>> ();

	auto &triangles = face_properties[triangle_key]
		.as <std::vector <glm::ivec3>> ();

	for (size_t i = 0; i < triangles.size(); i++) {
		auto &tri = triangles[i];

		auto &p0 = positions[tri.x];
		auto &p1 = positions[tri.y];
		auto &p2 = positions[tri.z];

		auto e1 = p1 - p0;
		auto e2 = p2 - p0;

		auto n = glm::normalize(glm::cross(e1, e2));

		normals[tri.x] += n;
		normals[tri.y] += n;
		normals[tri.z] += n;
	}

	for (auto &n : normals)
		n = glm::normalize(n);

	vertex_properties[normal_key] = normals;

	return *this;
}