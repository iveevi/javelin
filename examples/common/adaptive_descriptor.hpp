#pragma once

#include <vulkan/vulkan.hpp>

#include <bestd/tree.hpp>

class AdaptiveDescriptor {
	vk::DescriptorSet handle;
	vk::DescriptorSetLayout layout;

	// TODO: need to add binding information as well for each key
	bestd::tree <std::string, uint32_t> bindings;
	bestd::tree <std::string, uint32_t> missing;
public:
	// Common extra keys for material-centric descriptor sets
	static constexpr const char *environment_key = "Environment";

	AdaptiveDescriptor() : handle(VK_NULL_HANDLE) {}

	AdaptiveDescriptor &with_layout(const vk::DescriptorSetLayout &layout_) {
		layout = layout_;
		return *this;
	}

	void allocate(const vk::Device &device, const vk::DescriptorPool &pool) {
		// TODO: assertion
		auto allocate_info = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(pool)
			.setSetLayouts(layout);

		handle = device.allocateDescriptorSets(allocate_info).front();
	}

	const vk::DescriptorSet &set() const {
		return handle;
	}
	
	void requirement(const std::string &key, uint32_t binding) {
		missing[key] = binding;
		bindings[key] = binding;
	}
	
	void resolve(const std::string &key) {
		missing.erase(key);
	}

	auto dependencies() const {
		return missing;
	}
	
	auto mapped() const {
		return bindings;
	}

	uint32_t key_binding(const std::string &key) const {
		return bindings.at(key);
	}

	bool null() const {
		return static_cast <VkDescriptorSet> (handle) == VK_NULL_HANDLE;
	}

	bool complete() const {
		return missing.empty();
	}
};

using DescriptorTable = bestd::tree <std::string, AdaptiveDescriptor>;