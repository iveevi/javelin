#pragma once

#include "artifact.hpp"
#include "layouts.hpp"
#include "resources.hpp"
#include "error.hpp"

namespace jvl::tsg {

// Scanning specifiers for a fixed reference
template <specifier Ref, specifier ... Specifiers>
struct verify_specifier_atom {
	static constexpr auto value = shader_compiler_error(eOk);
};

// Specializations for layout inputs
template <ShaderStageFlags F, generic T, size_t N, specifier S, specifier ... Specifiers>
struct verify_specifier_atom <LayoutIn <F, T, N>, S, Specifiers...> {
	static constexpr auto value = verify_specifier_atom <LayoutIn <F, T, N>, Specifiers...> ::value;
};

template <generic T, size_t N, specifier ... Specifiers>
struct verify_specifier_atom <LayoutIn <ShaderStageFlags::eFragment, T, N>,
			      LayoutOut <ShaderStageFlags::eVertex, T, N>,
			      Specifiers...> {
	static constexpr auto value = shader_compiler_error(eOk);
};

template <generic T, generic U, size_t N, specifier ... Specifiers>
struct verify_specifier_atom <LayoutIn <ShaderStageFlags::eFragment, T, N>,
			      LayoutOut <ShaderStageFlags::eVertex, U, N>,
			      Specifiers...> {
	static constexpr auto value = shader_compiler_error(eIncompatibleLayout, N);
};

template <generic T, size_t N>
struct verify_specifier_atom <LayoutIn <ShaderStageFlags::eFragment, T, N>> {
	static constexpr auto value = shader_compiler_error(eMissingLayout, N);
};

template <typename T>
struct verify_specifiers {
	static constexpr auto value = shader_compiler_error(eUnknown);
};

// Evaluating error matrix for each 
template <specifier ... Specifiers>
struct verify_specifiers <std::tuple <Specifiers...>> {
	static constexpr size_t N = sizeof...(Specifiers);
	static constexpr std::array <shader_compiler_error, N> status {
		verify_specifier_atom <Specifiers, Specifiers...> ::value...
	};

	static constexpr shader_compiler_error assess() {
		constexpr size_t N = sizeof...(Specifiers);

		// Generate the error matrix
		std::array <shader_compiler_error, N> status {
			verify_specifier_atom <Specifiers, Specifiers...> ::value...
		};
		
		// Get the first problem if any
		for (size_t i = 0; i < N; i++) {
			if (status[i].type != eOk)
				return status[i];
		}

		return shader_compiler_error(eOk);
	}

	static constexpr shader_compiler_error value = assess();
};

} // namespace jvl::tsg