#pragma once

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

} // namespace jvl