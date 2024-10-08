#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/messaging.hpp"
#include "../../core/triangle_mesh.hpp"
#include "../../types.hpp"
#include "vertex_flags.hpp"

namespace jvl::gfx::vulkan {

struct InterleaveResult {
	buffer <float> buffer;
	VertexFlags enabled;
};

inline InterleaveResult interleave(const core::TriangleMesh &tmesh, VertexFlags flags)
{
	buffer <float> bf;

	size_t size = tmesh.positions.size();
	
	auto &positions = tmesh.positions;
	auto &normals = tmesh.normals;
	auto &uvs = tmesh.uvs;

	bool skip_normals = normals.empty();
	bool skip_uvs = uvs.empty();

	for (size_t i = 0; i < size; i++) {
		if (enabled(flags, VertexFlags::ePosition)) {
			assert(i < positions.size());

			auto &p = positions[i];
			bf.push_back(p.x);
			bf.push_back(p.y);
			bf.push_back(p.z);
		}

		if (!skip_normals && enabled(flags, VertexFlags::eNormal)) {
			assert(i < normals.size());

			auto &n = normals[i];
			bf.push_back(n.x);
			bf.push_back(n.y);
			bf.push_back(n.z);
		}
		
		if (!skip_uvs && enabled(flags, VertexFlags::eUV)) {
			assert(i < uvs.size());

			auto &t = uvs[i];
			bf.push_back(t.x);
			bf.push_back(t.y);
		}
	}

	if (skip_normals)
		flags = flags - VertexFlags::eNormal;
	
	if (skip_uvs)
		flags = flags - VertexFlags::eUV;

	return InterleaveResult(bf, flags);
}

struct TriangleMesh : core::Unique {
	VertexFlags flags;
	littlevk::Buffer vertices;
	
	size_t count;
	littlevk::Buffer triangles;
	
	std::set <int> material_usage;

	static std::optional <TriangleMesh> from(littlevk::LinkedDeviceAllocator <> allocator,
						 const core::TriangleMesh &tmesh,
						 const VertexFlags &maximal_flags = VertexFlags::eAll) {
		TriangleMesh vmesh;

		auto result = interleave(tmesh, maximal_flags);

		// fmt::println("result of interleaving: {} -- {:08b}",
		// 	result.buffer.size(),
		// 	int32_t(result.enabled));

		vmesh.uuid = core::new_uuid <TriangleMesh> ();
		vmesh.flags = result.enabled;
		vmesh.count = 3 * tmesh.triangles.size();

		std::tie(vmesh.vertices, vmesh.triangles) = allocator
			.buffer(result.buffer,
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
