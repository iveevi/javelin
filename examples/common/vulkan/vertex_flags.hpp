#pragma once

#include <cstdint>
#include <vector>

#include <littlevk/littlevk.hpp>

#include <math_types.hpp>
#include <logging.hpp>

namespace jvl::gfx::vulkan {

enum class VertexFlags : uint64_t {
	eNone		= 0b0,
	ePosition	= 0b1,
	eNormal		= 0b10,
	eUV		= 0b100,
	eAll		= ePosition | eNormal | eUV
};

[[gnu::always_inline]]
inline bool enabled(VertexFlags one, VertexFlags two)
{
	return (uint64_t(one) & uint64_t(two)) == uint64_t(two);
}

[[gnu::always_inline]]
inline VertexFlags operator|(VertexFlags one, VertexFlags two)
{
	return VertexFlags(uint64_t(one) | uint64_t(two));
}

[[gnu::always_inline]]
inline VertexFlags operator-(VertexFlags one, VertexFlags two)
{
	return VertexFlags(uint64_t(one) & ~uint64_t(two));
}

// Conversion to a version binding and attribute description
inline auto binding_and_attributes(VertexFlags flags)
{
	MODULE(binding-and-attributes);

	JVL_ASSERT_PLAIN(enabled(flags, vulkan::VertexFlags::ePosition));
	
	uint32_t offset = 0;
	uint32_t index = 0;

	std::vector <vk::VertexInputAttributeDescription> attributes;

	if (enabled(flags, vulkan::VertexFlags::ePosition)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32B32Sfloat, offset);
		flags = flags - vulkan::VertexFlags::ePosition;
		offset += sizeof(float3);
	}
	
	if (enabled(flags, vulkan::VertexFlags::eNormal)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32B32Sfloat, offset);
		flags = flags - vulkan::VertexFlags::eNormal;
		offset += sizeof(float3);
	}
	
	if (enabled(flags, vulkan::VertexFlags::eUV)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32Sfloat, offset);
		flags = flags - vulkan::VertexFlags::eUV;
		offset += sizeof(float2);
	}

	JVL_ASSERT(flags == vulkan::VertexFlags::eNone,
		"unhandled flags in vertex layout: {:08b}",
		uint32_t(flags));

	vk::VertexInputBindingDescription binding {
		0, offset, vk::VertexInputRate::eVertex
	};

	return std::make_tuple(binding, attributes);
}

} // namespace jvl::gfx::vulkan