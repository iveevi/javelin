#pragma once

#include "../scene.hpp"
#include "../ray.hpp"
#include "intersection.hpp"
#include "triangle_mesh.hpp"
#include "material.hpp"
#include "bvh.hpp"

namespace jvl::gfx::cpu {

struct Scene {
	std::vector <core::TriangleMesh> meshes;
	std::vector <core::Material> materials;
	BVH bvh;
	
	// Tracking correspondances from this scene to the original
	core::Equivalence <core::TriangleMesh, core::Scene::Object> mesh_to_object;

	void build_bvh();

	Intersection trace(const core::Ray &);

	static Scene from(const core::Scene &);
};

} // namespace jvl::gfx::cpu
