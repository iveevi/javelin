#pragma once

#include "vulkan_resources.hpp"

struct ShaderBindingTable {
	vk::StridedDeviceAddressRegionKHR ray_generation;
	vk::StridedDeviceAddressRegionKHR misses;
	vk::StridedDeviceAddressRegionKHR closest_hits;
	vk::StridedDeviceAddressRegionKHR callables;
};
	
std::pair <littlevk::Pipeline, ShaderBindingTable>
raytracing_pipeline(VulkanResources &,
		    const vk::ShaderModule &,
		    const std::vector <vk::ShaderModule> &,
		    const std::vector <vk::ShaderModule> &,
		    const std::vector <vk::DescriptorSetLayoutBinding> &,
		    const std::vector <vk::PushConstantRange> &);