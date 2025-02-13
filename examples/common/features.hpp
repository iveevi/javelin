#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

struct VulkanFeatureBase {
	VkStructureType sType;
	void *pNext;
};

struct VulkanFeatureChain : std::vector <VulkanFeatureBase *> {
	vk::PhysicalDeviceFeatures2KHR top;

	~VulkanFeatureChain() {
		for (auto &ptr : *this)
			delete ptr;
	}

	template <typename F>
	void add() {
		VulkanFeatureBase *ptr = (VulkanFeatureBase *) new F();

		// pNext chain
		if (size() > 0) {
			VulkanFeatureBase *pptr = back();
			pptr->pNext = ptr;
		} else {
			top.setPNext(ptr);
		}

		push_back(ptr);
	}
};

#define feature_case(Type)				\
	case (VkStructureType) Type::structureType:	\
		(*reinterpret_cast <Type *> (ptr))
