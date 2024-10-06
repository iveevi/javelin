#pragma once

#include <cstdint>

namespace jvl::gfx::vulkan {

enum class MaterialFlags : uint64_t {
	eNone = 0x0,
	eAlbedoSampler = 0x1,
	eSpecularSampler = 0x10,
	eRoughnessSampler = 0x100,
};

[[gnu::always_inline]]
inline bool enabled(MaterialFlags one, MaterialFlags two)
{
	return (uint64_t(one) & uint64_t(two)) == uint64_t(two);
}

[[gnu::always_inline]]
inline MaterialFlags operator|(MaterialFlags one, MaterialFlags two)
{
	return MaterialFlags(uint64_t(one) | uint64_t(two));
}

} // namespace jvl::gfx::vulkan