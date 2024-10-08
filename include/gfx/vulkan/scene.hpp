#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../cpu/scene.hpp"
#include "material.hpp"
#include "triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class SceneFlags : uint32_t {
	eDefault,		// Fine for most formats
	eOneMaterialPerMesh,	// Required for albedo texturing, etc.
};

struct Scene {
	std::vector <TriangleMesh> meshes;
	std::vector <Material> materials;
	SceneFlags flags;

	// Tracking correspondances from this scene to the original (disk)
	core::Equivalence <TriangleMesh, core::Scene::Object> mesh_to_object;

	// TODO: explicit update method... skips things already in the equivalence (which arent dirty)

	static Scene from(core::DeviceResourceCollection &, const cpu::Scene &, SceneFlags);
};

} // namespace jvl::gfx::vulkan