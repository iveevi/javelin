#pragma once

#include <vulkan/vulkan.hpp>

struct FormatInfo {
	size_t size;
	size_t channels;

	size_t bytes(const vk::Extent2D &) const;

	static FormatInfo fetch(const vk::Format &);
};