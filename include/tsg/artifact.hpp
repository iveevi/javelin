#pragma once

#include "layouts.hpp"
#include "imports.hpp"

namespace jvl::tsg {

// Set of all specifier types
template <typename T>
concept specifier = layout_input <T> || layout_output <T>;

// Type-safe wrapper of a tracked buffer
template <ShaderStageFlags, specifier ... Args>
struct CompiledArtifact : thunder::TrackedBuffer {};

template <ShaderStageFlags, typename ... Args>
struct compiled_artifact_assembler {
	static_assert(false);
};

template <ShaderStageFlags flags, layout_input ... Inputs, layout_output ... Outputs>
struct compiled_artifact_assembler <flags,
				    LayoutInputCollection <Inputs...>,
				    LayoutOutputCollection <Outputs...>> {
	static auto make() {
		return CompiledArtifact <flags, Inputs..., Outputs...> ();
	}
};

} // namespace jvl::tsg