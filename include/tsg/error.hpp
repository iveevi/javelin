#pragma once

#include "requirement.hpp"

namespace jvl::tsg {

enum shader_compiler_error_kind : int32_t {
	eOk,
	eUnknown,
	eIncompatibleLayouts,
	eUnusedOutputLayout,
	// TODO: missing layout...
};

struct shader_compiler_error {
	shader_compiler_error_kind type;
	int32_t arg0;

	static constexpr shader_compiler_error ok() {
		return shader_compiler_error(eOk);
	}
	
	static constexpr shader_compiler_error unknown() {
		return shader_compiler_error(eUnknown);
	}
	
	static constexpr shader_compiler_error bad_layout(int32_t x) {
		return shader_compiler_error(eIncompatibleLayouts, x);
	}
	
	static constexpr shader_compiler_error unused_layout(int32_t x) {
		return shader_compiler_error(eUnusedOutputLayout, x);
	}
};

template <typename Rs>
struct requirement_vector_error {
	static constexpr auto value = shader_compiler_error::ok();
};

template <requirement R, requirement ... Requirements>
struct requirement_vector_error <RequirementVector <R, Requirements...>> {
	static constexpr auto value = shader_compiler_error::unknown();
};

template <requirement ... Requirements>
struct requirement_vector_error <RequirementVector <RequirementSatisfied, Requirements...>> {
	using next = RequirementVector <Requirements...>;
	static constexpr auto value = requirement_vector_error <next> ::value;
};

template <ShaderStageFlags F, generic T, ShaderStageFlags B, size_t N, requirement ... Requirements>
struct requirement_vector_error <RequirementVector <RequirementCompatibleLayout <F, T, B, N>, Requirements...>> {
	static constexpr auto value = shader_compiler_error::bad_layout(N);
};

template <ShaderStageFlags From, generic T, ShaderStageFlags To, size_t N, requirement ... Requirements>
struct requirement_vector_error <RequirementVector <RequirementOutputUsage <From, T, To, N>, Requirements...>> {
	static constexpr auto value = shader_compiler_error::unused_layout(N);
};


template <int32_t binding_in_vertex_shader>
[[deprecated("vertex shader produces a layout ouput that is not consumed by the fragment shader")]]
constexpr void warn_unused_layout() {}

// Trigger static asserts conditionally
template <shader_compiler_error value>
constexpr void assess_shader_compiler_error()
{
	if constexpr (value.type == eUnknown) {
		static_assert(0,
			"unknown error while attempting to "
			"validate completed shader program");
	} else if constexpr (value.type == eIncompatibleLayouts) {
		constexpr int32_t problematic_binding_in_vs = value.arg0;
		static_assert(problematic_binding_in_vs == -1,
			"vertex shader produces a layout ouput of a different type "
			"than what the fragment shader expects");
	} else if constexpr (value.type == eUnusedOutputLayout) {
		constexpr int32_t source_binding_in_vs = value.arg0;
		warn_unused_layout <source_binding_in_vs> ();
	} else {
		static_assert(value.type == eOk);
	}
}

} // namespace jvl::tsg