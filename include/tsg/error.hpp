#pragma once

#include "requirement.hpp"

namespace jvl::tsg {

enum shader_compiler_error_kind : int32_t {
	eOk,
	eUnknown,
	eIncompatibleLayouts,
	eUnusedOutputLayout,
	eMultiplePushConstants,
	// TODO: missing layout...
};

struct shader_compiler_error {
	shader_compiler_error_kind type;
	int32_t arg0;
};

template <typename Rs>
struct verify_requirements_resolution {
	static constexpr auto value = shader_compiler_error(eOk);
};

template <requirement R, requirement ... Requirements>
struct verify_requirements_resolution <RequirementVector <R, Requirements...>> {
	static constexpr auto value = shader_compiler_error(eUnknown);
};

template <requirement ... Requirements>
struct verify_requirements_resolution <RequirementVector <RequirementSatisfied, Requirements...>> {
	using next = RequirementVector <Requirements...>;
	static constexpr auto value = verify_requirements_resolution <next> ::value;
};

template <ShaderStageFlags F, generic T, ShaderStageFlags B, size_t N, requirement ... Requirements>
struct verify_requirements_resolution <RequirementVector <RequirementCompatibleLayout <F, T, B, N>, Requirements...>> {
	static constexpr auto value = shader_compiler_error(eIncompatibleLayouts, N);
};

template <ShaderStageFlags From, generic T, ShaderStageFlags To, size_t N, requirement ... Requirements>
struct verify_requirements_resolution <RequirementVector <RequirementOutputUsage <From, T, To, N>, Requirements...>> {
	static constexpr auto value = shader_compiler_error(eUnusedOutputLayout, N);
};


template <int32_t binding_in_vertex_shader>
[[deprecated("vertex shader produces a layout ouput that is not consumed by the fragment shader")]]
constexpr void warn_unused_layout() {}

// Trigger static asserts conditionally
template <shader_compiler_error value>
consteval void assess_shader_compiler_error()
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
	} else if constexpr (value.type == eMultiplePushConstants) {
		static_assert(false, "shader functions can have at most "
			"one push constant uniform");
	} else {
		static_assert(value.type == eOk);
	}
}

} // namespace jvl::tsg