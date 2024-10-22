#pragma once

#include "artifact.hpp"
#include "artifact.hpp"

namespace jvl::tsg {

// Resulting pipeline type
template <ShaderStageFlags Flags, specifier ... Specifiers>
struct Pipeline {
	vk::Pipeline handle;
	vk::PipelineLayout layout;

	// TODO: descriptor set allocator; bind(device, pool, ppl).allocate...
};
 
} // namespace jvl::tsg