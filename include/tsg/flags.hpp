#pragma once

#include <cstdint>

namespace jvl::tsg {

// TODO: move to up include
// Generate default operations for enumeration classes
#define flag_operators(T, base)						\
	constexpr T operator+(T A, T B)					\
	{								\
		return T(base(A) | base(B));				\
	}								\
	constexpr bool has(T A, T B)					\
	{								\
		return ((base(A) & base(B)) == base(B));		\
	}

// Vulkan shader stages
enum class ShaderStageFlags : uint16_t {
	eNone		= 0,
	eVertex		= 1 << 0,
	eFragment	= 1 << 1,
	eCompute	= 1 << 2,
	eTask		= 1 << 3,
	eMesh		= 1 << 4,
	eSubroutine	= 1 << 5,
	
	// Composites
	eGraphicsVertexFragment	= eVertex | eFragment,
};

flag_operators(ShaderStageFlags, uint16_t)

// Error handling for signature-stage checking
enum class ClassificationErrorFlags : uint16_t {
	eOk			= 0,
	eDuplicateVertex	= 1 << 0,
	eDuplicateFragment	= 1 << 1,
	eMultipleStages		= 1 << 2,
};

flag_operators(ClassificationErrorFlags, uint16_t)

} // namespace jvl::tsg