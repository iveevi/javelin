#pragma once

#include <littlevk/littlevk.hpp>

#include "../../types.hpp"
#include "../../core/triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

struct TriangleMesh {
	littlevk::Buffer vertices;
	littlevk::Buffer triangles;
	size_t count;
	
	std::set <int> material_usage;

	// TODO: flags for the properties interleaved in vertices

	static std::optional <TriangleMesh> from(littlevk::LinkedDeviceAllocator <> allocator, const core::TriangleMesh &tmesh) {
		TriangleMesh vmesh;

		vmesh.count = 3 * tmesh.triangles.size();
		std::tie(vmesh.vertices, vmesh.triangles) = allocator
			.buffer(tmesh.positions,
				vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eVertexBuffer)
			.buffer(tmesh.triangles,
				vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eIndexBuffer);

		vmesh.material_usage = tmesh.material_usage;

		return vmesh;
	}
};

} // namespace jvl::gfx::vulkan
