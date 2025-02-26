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

// TODO: target stage...

} // namespace jvl