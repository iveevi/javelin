#pragma once

#include <littlevk/littlevk.hpp>

#include "triangle_mesh.hpp"
#include "../cpu/scene.hpp"

namespace jvl::gfx::vulkan {

struct Scene {
	std::vector <int64_t> uuids;
	std::vector <TriangleMesh> meshes;

	static Scene from(const littlevk::LinkedDeviceAllocator <> &allocator, const cpu::Scene &other) {
		Scene result;
		result.uuids = other.uuids;
		
		for (auto &m : other.meshes) {
			auto vkm = TriangleMesh::from(allocator, m).value();
			result.meshes.push_back(vkm);
		}

		return result;
	}
};

} // namespace jvl::gfx::vulkan