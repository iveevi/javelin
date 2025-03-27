#pragma once

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include "vulkan_resources.hpp"
#include "vulkan_triangle_mesh.hpp"

vk::TransformMatrixKHR vk_transform(const glm::mat4 &);

struct InstanceInfo {
	uint32_t index;
	uint32_t mask;
};

struct VulkanAccelerationStructure {
	vk::AccelerationStructureKHR handle;
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	vk::DeviceSize size;

	void destroy(const vk::Device &);

	static VulkanAccelerationStructure blas(VulkanResources &,
		const vk::CommandBuffer &,
		const VulkanTriangleMesh &,
		size_t);

	static VulkanAccelerationStructure tlas(VulkanResources &,
		const vk::CommandBuffer &,
		const std::vector <VulkanAccelerationStructure> &,
		const std::vector <vk::TransformMatrixKHR> &,
		const std::vector <InstanceInfo> &);
};
