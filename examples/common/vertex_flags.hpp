#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include <littlevk/littlevk.hpp>

#include <common/logging.hpp>

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

	JVL_ASSERT_PLAIN(enabled(flags, VertexFlags::ePosition));
	
	uint32_t offset = 0;
	uint32_t index = 0;

	std::vector <vk::VertexInputAttributeDescription> attributes;

	if (enabled(flags, VertexFlags::ePosition)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32B32Sfloat, offset);
		flags = flags - VertexFlags::ePosition;
		offset += sizeof(glm::vec3);
	}
	
	if (enabled(flags, VertexFlags::eNormal)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32B32Sfloat, offset);
		flags = flags - VertexFlags::eNormal;
		offset += sizeof(glm::vec3);
	}
	
	if (enabled(flags, VertexFlags::eUV)) {
		attributes.emplace_back(index++, 0, vk::Format::eR32G32Sfloat, offset);
		flags = flags - VertexFlags::eUV;
		offset += sizeof(glm::vec2);
	}

	JVL_ASSERT(flags == VertexFlags::eNone,
		"unhandled flags in vertex layout: {:08b}",
		uint32_t(flags));

	vk::VertexInputBindingDescription binding {
		0, offset, vk::VertexInputRate::eVertex
	};

	return std::make_tuple(binding, attributes);
}