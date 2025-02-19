#include "vulkan_triangle_mesh.hpp"

static std::vector <float> interleave(const TriangleMesh &tmesh, VertexFlags flags)
{
	std::vector <float> buffer;

	size_t size = tmesh.positions.size();
	
	auto &positions = tmesh.positions;
	auto &normals = tmesh.normals;
	auto &uvs = tmesh.uvs;

	for (size_t i = 0; i < size; i++) {
		if (enabled(flags, VertexFlags::ePosition)) {
			assert(i < positions.size());

			auto &p = positions[i];
			buffer.push_back(p.x);
			buffer.push_back(p.y);
			buffer.push_back(p.z);
		}

		if (enabled(flags, VertexFlags::eNormal)) {
			if (normals.empty()) {
				buffer.push_back(0);
				buffer.push_back(0);
				buffer.push_back(0);
			} else {
				auto &n = normals[i];
				buffer.push_back(n.x);
				buffer.push_back(n.y);
				buffer.push_back(n.z);
			}
		}
		
		if (enabled(flags, VertexFlags::eUV)) {
			if (uvs.empty()) {
				buffer.push_back(0);
				buffer.push_back(0);
			} else {
				auto &t = uvs[i];
				buffer.push_back(t.x);
				buffer.push_back(t.y);
			}
		}
	}

	return buffer;
}
	
bestd::optional <VulkanTriangleMesh> VulkanTriangleMesh::from(littlevk::LinkedDeviceAllocator <> allocator,
							      const ::TriangleMesh &tmesh,
							      const VertexFlags &flags,
							      const vk::BufferUsageFlags &extra)
{
	VulkanTriangleMesh vmesh;

	auto buffer = interleave(tmesh, flags);

	vmesh.flags = flags;
	vmesh.triangle_count = tmesh.triangles.size();
	vmesh.vertex_count = tmesh.positions.size();

	std::tie(vmesh.vertices, vmesh.triangles) = allocator
		.buffer(buffer, extra
			| vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eVertexBuffer)
		.buffer(tmesh.triangles, extra
			| vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eIndexBuffer);

	vmesh.material_usage = tmesh.material_usage;

	return vmesh;
}