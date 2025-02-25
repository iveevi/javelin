#pragma once

#include <set>

#include "imported_mesh.hpp"

#include <fmt/printf.h>

struct TriangleMesh {
	std::vector <glm::vec3> positions;
	std::vector <glm::vec3> normals;
	std::vector <glm::vec2> uvs;
	std::vector <glm::ivec3> triangles;
	std::vector <int> materials;
	std::set <int> material_usage;

	TriangleMesh() = default;

	std::pair <glm::vec3, glm::vec3> scale() const;

	static std::optional <TriangleMesh> from(const ImportedMesh &);

	static TriangleMesh uv_sphere(int32_t, float);
};