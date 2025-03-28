#pragma once

#include "imports.hpp"

namespace jvl::tsg {

// Shader stage intrinsics, used to indicate the
// shader stage of a shader program metafunction
struct VertexIntrinsics {};
struct FragmentIntrinsics {};
// TODO: put discard here ^
struct ComputerIntrinsics {};

// TODO: template structure for a result with mesh shaders...

// Interpolation mode wrappers
template <generic T>
struct Flat {};

template <generic T>
struct Smooth {};

template <generic T>
struct NoPerspective;

// Shader layout resources
template <generic T, size_t Offset = 0>
struct PushConstant : ire::push_constant <T> {
	PushConstant() = default;
	PushConstant(const ire::push_constant <T> &other)
			: ire::push_constant <T> (other) {}
};

// Result types for shaders
struct Position : vec4 {
	using vec4::vec4;

	Position(const vec4 &other) : vec4(other.ref) {}
};

} // namespace jvl::tsg