#pragma once

#include "../../core/triangle_mesh.hpp"
#include "../../core/material.hpp"
#include "../../core/scene.hpp"
#include "bvh.hpp"

namespace jvl::gfx::cpu {

struct Scene {
	std::vector <int64_t> uuids;
	std::vector <core::TriangleMesh> meshes;
	std::vector <core::Material> materials;
	BVH bvh;

	void build_bvh();

	Intersection trace(const core::Ray &);

	static Scene from(const core::Scene &);
};

} // namespace jvl::gfx::cpu
