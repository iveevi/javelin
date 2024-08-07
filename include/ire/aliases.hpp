#pragma once

#include "type_synthesis.hpp"
#include "gltype.hpp"
#include "vector.hpp"
#include "matrix.hpp"

namespace jvl::ire {

// Common types
using i32 = gltype <int>;
using f32 = gltype <float>;
using boolean = gltype <bool>;

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
constexpr op::PrimitiveType primitive_type()
{
	if constexpr (std::is_same_v <T, bool>)
		return op::boolean;
	if constexpr (std::is_same_v <T, int>)
		return op::i32;
	if constexpr (std::is_same_v <T, float>)
		return op::f32;

	if constexpr (std::is_same_v <T, gltype <bool>>)
		return op::boolean;
	if constexpr (std::is_same_v <T, gltype <int>>)
		return op::i32;
	if constexpr (std::is_same_v <T, gltype <float>>)
		return op::f32;

	if constexpr (std::is_same_v <T, vec2>)
		return op::vec2;
	if constexpr (std::is_same_v <T, vec3>)
		return op::vec3;
	if constexpr (std::is_same_v <T, vec4>)
		return op::vec4;

	if constexpr (std::is_same_v <T, ivec2>)
		return op::ivec2;
	if constexpr (std::is_same_v <T, ivec3>)
		return op::ivec3;
	if constexpr (std::is_same_v <T, ivec4>)
		return op::ivec4;

	if constexpr (std::is_same_v <T, mat2>)
		return op::mat2;
	if constexpr (std::is_same_v <T, mat3>)
		return op::mat3;
	if constexpr (std::is_same_v <T, mat4>)
		return op::mat4;

	return op::bad;
}

} // namespace jvl::ire
