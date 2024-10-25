#pragma once

#include <map>
#include <cstdint>

#include <vulkan/vulkan.hpp>

#include "../logging.hpp"

namespace jvl::core {

struct FormatInfo {
	size_t size;
	size_t channels;

	size_t bytes(const vk::Extent2D &) const;

	static FormatInfo fetch(const vk::Format &);
};

} // namespace jvl::core