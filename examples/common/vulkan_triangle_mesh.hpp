#pragma once

#include <littlevk/littlevk.hpp>

#include "triangle_mesh.hpp"
#include "vertex_flags.hpp"

struct VulkanTriangleMesh {
	littlevk::Buffer vertices;
	littlevk::Buffer triangles;

	size_t vertex_count;
	size_t triangle_count;

	VertexFlags flags;

	std::set <int> material_usage;

	VulkanTriangleMesh() = default;

	static bestd::optional <VulkanTriangleMesh> from(littlevk::LinkedDeviceAllocator <>,
							 const ::TriangleMesh &,
							 const VertexFlags & = VertexFlags::eAll,
							 const vk::BufferUsageFlags & = vk::BufferUsageFlagBits(0));
};
