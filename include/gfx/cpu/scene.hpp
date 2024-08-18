#pragma once

#include "../../core/triangle_mesh.hpp"
#include "../../core/material.hpp"
#include "../../core/preset.hpp"
#include "bvh.hpp"

namespace jvl::gfx::cpu {

struct Scene {
	std::vector <core::TriangleMesh> meshes;
	std::vector <core::Material> materials;
	BVH bvh;

	void add(const core::Preset &);
	void build_bvh();

	Intersection trace(const core::Ray &);
};

} // namespace jvl::gfx::cpu
