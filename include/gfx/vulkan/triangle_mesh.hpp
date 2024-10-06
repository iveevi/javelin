#pragma once

#include <littlevk/littlevk.hpp>

#include "../../types.hpp"
#include "../../core/triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class VertexFlags : uint64_t {
	eNone = 0x0,
	eVertex = 0x1,
	eNormal = 0x10,
	eUV = 0x100,
	eAll = eVertex | eNormal | eUV
};

struct TriangleMesh {
	littlevk::Buffer vertices;
	littlevk::Buffer triangles;
	size_t count;
	
	std::set <int> material_usage;

	static std::optional <TriangleMesh> from(littlevk::LinkedDeviceAllocator <> allocator,
						 const core::TriangleMesh &tmesh,
						 const VertexFlags &flags = VertexFlags::eAll) {
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
