#pragma once

#include <cstdint>

enum class MaterialFlags : uint64_t {
	eNone = 0b0,
	eAlbedoSampler = 0b1,
	eSpecularSampler = 0b10,
	eRoughnessSampler = 0b100,
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