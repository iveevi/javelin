#pragma once

#include "../scene.hpp"
#include "triangle_mesh.hpp"
#include "material.hpp"

namespace jvl::cpu {

struct Scene {
	std::vector <core::TriangleMesh> meshes;
	std::vector <core::Material> materials;
	
	// Tracking correspondances from this scene to the original
	core::Equivalence <core::TriangleMesh, core::Scene::Object> mesh_to_object;

	static Scene from(const core::Scene &);
};

} // namespace jvl::cpu
