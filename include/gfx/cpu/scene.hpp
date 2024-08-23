#pragma once

#include "../../core/triangle_mesh.hpp"
#include "../../core/material.hpp"
#include "bvh.hpp"

// Forward declarations
namespace jvl::engine {

struct ImportedAsset;

}

// Primary declaration
namespace jvl::gfx::cpu {

struct Scene {
	std::vector <core::TriangleMesh> meshes;
	std::vector <core::Material> materials;
	BVH bvh;

	// TODO: build from core::Scene
	void add(const engine::ImportedAsset &);
	void build_bvh();

	Intersection trace(const core::Ray &);
};

} // namespace jvl::gfx::cpu
