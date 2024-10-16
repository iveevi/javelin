#pragma once

#include "requirement.hpp"

namespace jvl::tsg {

enum shader_compiler_error_kind : int32_t {
	eOk,
	eUnknown,
	eIncompatibleLayouts
	// TODO: missing layout...
	// TODO: unused layout (warning)...
};

// TODO: enum for type of error
struct shader_compiler_error {
	shader_compiler_error_kind type;
	int32_t expected_binding;

	static constexpr shader_compiler_error ok() {
		return shader_compiler_error(eOk);
	}
	
	static constexpr shader_compiler_error unknown() {
		return shader_compiler_error(eUnknown);
	}
	
	static constexpr shader_compiler_error bad_layout(int32_t x) {
		return shader_compiler_error(eIncompatibleLayouts, x);
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

// Trigger static asserts conditionally
template <shader_compiler_error value>
constexpr void assess_shader_compiler_error()
{
	if constexpr (value.type == eUnknown) {
		static_assert(0,
			"unknown error while attempting to "
			"validate (Fragment << Vertex) program");
	} else if constexpr (value.type == eIncompatibleLayouts) {
		constexpr int32_t problematic_binding_in_vs = value.expected_binding;
		static_assert(problematic_binding_in_vs == -1,
			"vertex shader produces a layout ouput of a different type "
			"than what the fragment shader expects");
	} else {
		static_assert(value.type == eOk);
	}
}

} // namespace jvl::tsg