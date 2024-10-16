#pragma once

#include "imports.hpp"

namespace jvl::tsg {

// Shader stage intrinsics, used to indicate the
// shader stage of a shader program metafunction
struct vertex_intrinsics {};
struct fragment_intrinsics {};
// TODO: put discard here ^
struct compute_intrinsics {};

// Result types for shaders
struct position : vec4 {
	using vec4::vec4;
};

// TODO: template structure for a result with mesh shaders...

// Interpolation mode wrappers
template <generic T>
struct flat {};

template <generic T>
struct smooth {};

template <generic T>
struct noperspective;

} // namespace jvl::tsg