#pragma once

#include "type_synthesis.hpp"
#include "primitive.hpp"
#include "vector.hpp"
#include "matrix.hpp"

namespace jvl::ire {

// Common types
using i32 = primitive_t <int>;
using f32 = primitive_t <float>;
using boolean = primitive_t <bool>;

using ivec2 = vec <int, 2>;
using ivec3 = vec <int, 3>;
using ivec4 = vec <int, 4>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat2 = mat <float, 2, 2>;
using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

// Type matching
template <typename T>
constexpr thunder::PrimitiveType synthesize_primitive_type()
{
	if constexpr (std::is_same_v <T, bool>)
		return thunder::boolean;
	if constexpr (std::is_same_v <T, int>)
		return thunder::i32;
	if constexpr (std::is_same_v <T, float>)
		return thunder::f32;

	if constexpr (std::is_same_v <T, primitive_t <bool>>)
		return thunder::boolean;
	if constexpr (std::is_same_v <T, primitive_t <int>>)
		return thunder::i32;
	if constexpr (std::is_same_v <T, primitive_t <float>>)
		return thunder::f32;

	if constexpr (std::is_same_v <T, vec2>)
		return thunder::vec2;
	if constexpr (std::is_same_v <T, vec3>)
		return thunder::vec3;
	if constexpr (std::is_same_v <T, vec4>)
		return thunder::vec4;

	if constexpr (std::is_same_v <T, ivec2>)
		return thunder::ivec2;
	if constexpr (std::is_same_v <T, ivec3>)
		return thunder::ivec3;
	if constexpr (std::is_same_v <T, ivec4>)
		return thunder::ivec4;

	if constexpr (std::is_same_v <T, mat2>)
		return thunder::mat2;
	if constexpr (std::is_same_v <T, mat3>)
		return thunder::mat3;
	if constexpr (std::is_same_v <T, mat4>)
		return thunder::mat4;

	return thunder::bad;
}

} // namespace jvl::ire
