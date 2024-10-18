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

///////////////////////////////////////
// Specializations for layout inputs //
///////////////////////////////////////

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

// Vertex layout inputs are not expected to be resolved
template <generic T, size_t N>
struct verify_specifier_atom <LayoutIn <ShaderStageFlags::eFragment, T, N>> {
	static constexpr auto value = shader_compiler_error(eMissingLayout, N);
};

////////////////////////////////////////
// Specializations for layout outputs //
////////////////////////////////////////

template <ShaderStageFlags F, generic T, size_t N, specifier S, specifier ... Specifiers>
struct verify_specifier_atom <LayoutOut <F, T, N>, S, Specifiers...> {
	static constexpr auto value = verify_specifier_atom <LayoutOut  <F, T, N>, Specifiers...> ::value;
};

template <generic T, size_t N, specifier ... Specifiers>
struct verify_specifier_atom <LayoutOut <ShaderStageFlags::eVertex, T, N>,
			      LayoutIn <ShaderStageFlags::eFragment, T, N>,
			      Specifiers...> {
	static constexpr auto value = shader_compiler_error(eOk);
};

// Ignore this for fragment shader outputs
template <generic T, size_t N>
struct verify_specifier_atom <LayoutOut <ShaderStageFlags::eVertex, T, N>> {
	static constexpr auto value = shader_compiler_error(eUnusedOutputLayout, N);
};

////////////////////////////////////////
// Specializations for push constants //
////////////////////////////////////////

template <ShaderStageFlags F,
	  generic T, size_t Begin, size_t End,
	  specifier S,
	  specifier ... Specifiers>
struct verify_specifier_atom <PushConstantRange <F, T, Begin, End>, S, Specifiers...> {
	static constexpr auto value = verify_specifier_atom <PushConstantRange <F, T, Begin, End>, Specifiers...> ::value;
};

// Handle the self case early
template <ShaderStageFlags F,
	  generic T, size_t Begin, size_t End,
	  specifier ... Specifiers>
struct verify_specifier_atom <PushConstantRange <F, T, Begin, End>,
			      PushConstantRange <F, T, Begin, End>,
			      Specifiers...> {
	static constexpr auto value = verify_specifier_atom <PushConstantRange <F, T, Begin, End>, Specifiers...> ::value;
};

// Conflicting push constants
template <ShaderStageFlags F,
	  generic T, size_t Begin0, size_t End0,
	  generic U, size_t Begin1, size_t End1,
	  specifier ... Specifiers>
struct verify_specifier_atom <PushConstantRange <F, T, Begin0, End0>,
			      PushConstantRange <F, U, Begin1, End1>,
			      Specifiers...> {
	static constexpr auto value = shader_compiler_error(eMultiplePushConstants);
};

// Overlapping ranging from two different stages;
// for now only checks between the vertex and fragment stages
template <generic T, size_t Begin0, size_t End0,
	  generic U, size_t Begin1, size_t End1,
	  specifier ... Specifiers>
struct verify_specifier_atom <PushConstantRange <ShaderStageFlags::eVertex, T, Begin0, End0>,
			      PushConstantRange <ShaderStageFlags::eFragment, U, Begin1, End1>,
			      Specifiers...> {
	static constexpr auto B0 = Begin0;
	static constexpr auto B1 = Begin1;

	static constexpr auto assess() {
		constexpr auto overlapping = false
			|| (Begin0 < Begin1 && End0 > Begin1)
			|| (Begin1 < Begin0 && End1 > Begin0);
		if (overlapping)
			return shader_compiler_error(eOverlappingPushConstants);

		return verify_specifier_atom <PushConstantRange <ShaderStageFlags::eVertex, T, Begin0, End0>, Specifiers...> ::value;
	}

	static constexpr auto value = assess();
};

// Tuple acceptor
template <typename T>
struct verify_specifiers {
	static constexpr auto value = shader_compiler_error(eUnknown);
};

// Evaluating error matrix for each 
template <specifier ... Specifiers>
struct verify_specifiers <std::tuple <Specifiers...>> {
	static constexpr shader_compiler_error assess() {
		constexpr size_t N = sizeof...(Specifiers);

		// Generate the error matrix
		std::array <shader_compiler_error, N> status {
			verify_specifier_atom <Specifiers, Specifiers...> ::value...
		};
		
		// Get the first problem if any
		// TODO: check for errors first...
		for (size_t i = 0; i < N; i++) {
			if (status[i].type != eOk)
				return status[i];
		}

		return shader_compiler_error(eOk);
	}

	static constexpr shader_compiler_error value = assess();
};

} // namespace jvl::tsg