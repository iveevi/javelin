#pragma once

#include <cstdint>

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

} // namespace jvl::gfx::vulkan