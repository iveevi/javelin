#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <bestd/variant.hpp>

namespace jvl {

// Target generation modes
enum class Target {
	glsl,
	slang,
	cplusplus,
	cuda,
	cuda_nvrtc,
	spirv_assembly,
	spirv_binary,
	spirv_binary_via_glsl,
	__end,
};

// Target pipeline stages
enum class Stage {
	vertex,
	fragment,
	compute,
	mesh,
	task,
	ray_generation,
	intersection,
	any_hit,
	closest_hit,
	miss,
	callable,
	__end,
};

// Target generation results
using SourceResult = std::string;
using BinaryResult = std::vector <uint32_t>;
using FunctionResult = void *;

using GeneratedResult = bestd::variant <SourceResult, BinaryResult, FunctionResult>;

} // namespace jvl