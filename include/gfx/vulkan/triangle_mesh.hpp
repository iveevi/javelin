#pragma once

#include <littlevk/littlevk.hpp>

#include "../../types.hpp"
#include "../../core/triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class VertexFlags : uint64_t {
	eNone		= 0x0,
	ePosition	= 0x1,
	eNormal		= 0x10,
	eUV		= 0x100,
	eAll		= ePosition | eNormal | eUV
};

[[gnu::always_inline]]
inline bool enabled(VertexFlags one, VertexFlags two)
{
	return (uint64_t(one) & uint64_t(two)) == uint64_t(two);
}

[[gnu::always_inline]]
inline VertexFlags operator|(VertexFlags one, VertexFlags two)
{
	return VertexFlags(uint64_t(one) | uint64_t(two));
}

inline buffer <float> interleave(const core::TriangleMesh &tmesh, const VertexFlags &flags)
{
	buffer <float> bf;

	size_t size = tmesh.positions.size();
	
	auto &positions = tmesh.positions;
	auto &normals = tmesh.normals;
	auto &uvs = tmesh.uvs;

	for (size_t i = 0; i < size; i++) {
		if (enabled(flags, VertexFlags::ePosition)) {
			assert(i < positions.size());

			auto &p = positions[i];
			bf.push_back(p.x);
			bf.push_back(p.y);
			bf.push_back(p.z);
		}

		if (enabled(flags, VertexFlags::eNormal)) {
			assert(i < normals.size());

			auto &n = normals[i];
			bf.push_back(n.x);
			bf.push_back(n.y);
			bf.push_back(n.z);
		}
		
		if (enabled(flags, VertexFlags::eUV)) {
			assert(i < uvs.size());

			auto &t = uvs[i];
			bf.push_back(t.x);
			bf.push_back(t.y);
		}
	}

	return bf;
}

struct TriangleMesh {
	littlevk::Buffer vertices;
	littlevk::Buffer triangles;
	size_t count;
	
	std::set <int> material_usage;

	VertexFlags flags;

	static std::optional <TriangleMesh> from(littlevk::LinkedDeviceAllocator <> allocator,
						 const core::TriangleMesh &tmesh,
						 const VertexFlags &flags = VertexFlags::eAll) {
		TriangleMesh vmesh;

		vmesh.flags = flags;
		vmesh.count = 3 * tmesh.triangles.size();

		std::tie(vmesh.vertices, vmesh.triangles) = allocator
			.buffer(interleave(tmesh, flags),
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
