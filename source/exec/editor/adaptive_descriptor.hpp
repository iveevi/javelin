#pragma once

#include <set>

#include <vulkan/vulkan.hpp>

class AdaptiveDescriptor : public vk::DescriptorSet {
	// TODO: need to add binding information as well for each key
	std::set <std::string> missing;
public:
	// Common extra keys for material-centric descriptor sets
	static constexpr const char *environment_key = "Environment";

	AdaptiveDescriptor(const vk::DescriptorSet &set = VK_NULL_HANDLE)
		: vk::DescriptorSet(set) {}

	AdaptiveDescriptor &operator=(const vk::DescriptorSet &other) {
		vk::DescriptorSet::operator=(other);
		return *this;
	}
	
	void requirement(const std::string &key) {
		missing.insert(key);
	}
	
	void resolve(const std::string &key) {
		missing.erase(key);
	}

	auto dependencies() const {
		return missing;
	}

	bool null() const {
		return static_cast <vk::DescriptorSet> (*this) == VK_NULL_HANDLE;
	}

	bool complete() const {
		return missing.empty();
	}
};