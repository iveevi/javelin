#include "imported_mesh.hpp"

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