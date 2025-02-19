#pragma once

#include <littlevk/littlevk.hpp>

#include "triangle_mesh.hpp"
#include "vertex_flags.hpp"

struct InterleaveResult {
	std::vector <float> data;
	VertexFlags enabled;
};

inline InterleaveResult interleave(const TriangleMesh &tmesh, VertexFlags flags)
{
	std::vector <float> bf;

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

		if (enabled(flags, VertexFlags::eNormal)) {
			if (normals.empty()) {
				bf.push_back(0);
				bf.push_back(0);
				bf.push_back(0);
			} else {
				auto &n = normals[i];
				bf.push_back(n.x);
				bf.push_back(n.y);
				bf.push_back(n.z);
			}
		}
		
		if (enabled(flags, VertexFlags::eUV)) {
			if (uvs.empty()) {
				bf.push_back(0);
				bf.push_back(0);
			} else {
				auto &t = uvs[i];
				bf.push_back(t.x);
				bf.push_back(t.y);
			}
		}
	}

	return InterleaveResult(bf, flags);
}

struct VulkanTriangleMesh {
	VertexFlags flags;
	littlevk::Buffer vertices;
	
	size_t count;
	littlevk::Buffer triangles;
	
	std::set <int> material_usage;

	VulkanTriangleMesh() = default;

	static bestd::optional <VulkanTriangleMesh> from(littlevk::LinkedDeviceAllocator <> allocator,
							 const ::TriangleMesh &tmesh,
							 const VertexFlags &maximal_flags = VertexFlags::eAll,
							 const vk::BufferUsageFlags &extra = vk::BufferUsageFlagBits(0)) {
		VulkanTriangleMesh vmesh;

		auto result = interleave(tmesh, maximal_flags);

		vmesh.flags = result.enabled;
		vmesh.count = 3 * tmesh.triangles.size();

		std::tie(vmesh.vertices, vmesh.triangles) = allocator
			.buffer(result.data, extra
				| vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eVertexBuffer)
			.buffer(tmesh.triangles, extra
				| vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eIndexBuffer);

		vmesh.material_usage = tmesh.material_usage;

		return vmesh;
	}
};