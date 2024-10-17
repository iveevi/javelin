#pragma once

#include <cstdint>

namespace jvl::tsg {

enum shader_compiler_error_kind : int32_t {
	eOk,
	eUnknown,
	eIncompatibleLayout,
	eMissingLayout,
	eUnusedOutputLayout,
	eMultiplePushConstants,
	eOverlappingPushConstants,
};

struct shader_compiler_error {
	shader_compiler_error_kind type;
	int32_t arg0;
};

template <int32_t binding_in_vertex_shader>
[[deprecated("vertex shader produces a layout ouput that is not consumed by the fragment shader")]]
constexpr void warn_unused_layout() {}

// Trigger static asserts conditionally
template <shader_compiler_error value>
consteval void diagnose_shader_compiler_error()
{
	if constexpr (value.type == eUnknown) {
		static_assert(0,
			"unknown error while attempting to "
			"validate completed shader program");
	} else if constexpr (value.type == eIncompatibleLayout) {
		constexpr int32_t problematic_binding_in_vs = value.arg0;
		static_assert(problematic_binding_in_vs == -1,
			"vertex shader produces a layout ouput of a different type "
			"than what the fragment shader expects");
	} else if constexpr (value.type == eMissingLayout) {
		constexpr int32_t missing_binding_from_fs = value.arg0;
		static_assert(missing_binding_from_fs == -1,
			"fragment shader takes a layout input which is "
			"missing from the vertex shader");
	} else if constexpr (value.type == eUnusedOutputLayout) {
		constexpr int32_t source_binding_in_vs = value.arg0;
		warn_unused_layout <source_binding_in_vs> ();
	} else if constexpr (value.type == eMultiplePushConstants) {
		static_assert(false, "shader functions can have at most "
			"one push constant uniform");
	} else if constexpr (value.type == eOverlappingPushConstants) {
		static_assert(false, "shader program contains stages that "
			"define push constants with overlapping ranges");
	} else {
		static_assert(value.type == eOk);
	}
}

} // namespace jvl::tsg