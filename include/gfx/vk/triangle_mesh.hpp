#pragma once

#include <littlevk/littlevk.hpp>

#include "../../types.hpp"
#include "../../core/triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

struct TriangleMesh {
	littlevk::Buffer vertices;
	littlevk::Buffer triangles;
	size_t count;

	// property<littlevk::Buffer> attributes;

	static std::optional <TriangleMesh> from(littlevk::LinkedDeviceAllocator <> &allocator, const core::TriangleMesh &tmesh) {
		TriangleMesh vmesh;

		vmesh.count = 3 * tmesh.triangles.size();
		std::tie(vmesh.vertices, vmesh.triangles) = allocator
			.buffer(tmesh.positions, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
			.buffer(tmesh.triangles, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);

		return vmesh;
	}
};

} // namespace jvl::gfx::vulkan
