#pragma once

#include "native.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "sampler.hpp"
#include "image.hpp"

namespace jvl::ire {

// Aliases for common primitive types
using i32 = native_t <int32_t>;
using u32 = native_t <uint32_t>;
using f32 = native_t <float>;
using boolean = native_t <bool>;

using ivec2 = vec <int32_t, 2>;
using ivec3 = vec <int32_t, 3>;
using ivec4 = vec <int32_t, 4>;

using uvec2 = vec <uint32_t, 2>;
using uvec3 = vec <uint32_t, 3>;
using uvec4 = vec <uint32_t, 4>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat2 = mat <float, 2, 2>;
using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

// Aliases for samplers
using isampler1D = sampler <int32_t, 1>;
using isampler2D = sampler <int32_t, 2>;
using isampler3D = sampler <int32_t, 3>;

using usampler1D = sampler <uint32_t, 1>;
using usampler2D = sampler <uint32_t, 2>;
using usampler3D = sampler <uint32_t, 3>;

using sampler1D = sampler <float, 1>;
using sampler2D = sampler <float, 2>;
using sampler3D = sampler <float, 3>;

// Aliases for images
using iimage1D = image <int32_t, 1>;
using iimage2D = image <int32_t, 2>;
using iimage3D = image <int32_t, 3>;

using uimage1D = image <uint32_t, 1>;
using uimage2D = image <uint32_t, 2>;
using uimage3D = image <uint32_t, 3>;

using image1D = image <float, 1>;
using image2D = image <float, 2>;
using image3D = image <float, 3>;

// // Type matching
// template <typename T>
// constexpr thunder::PrimitiveType synthesize_primitive_type()
// {
// 	if constexpr (std::is_same_v <T, bool>)
// 		return thunder::boolean;
// 	if constexpr (std::is_same_v <T, int32_t>)
// 		return thunder::i32;
// 	if constexpr (std::is_same_v <T, uint32_t>)
// 		return thunder::u32;
// 	if constexpr (std::is_same_v <T, float>)
// 		return thunder::f32;

// 	if constexpr (std::is_same_v <T, native_t <bool>>)
// 		return thunder::boolean;
// 	if constexpr (std::is_same_v <T, native_t <int32_t>>)
// 		return thunder::i32;
// 	if constexpr (std::is_same_v <T, native_t <uint32_t>>)
// 		return thunder::u32;
// 	if constexpr (std::is_same_v <T, native_t <float>>)
// 		return thunder::f32;

// 	if constexpr (std::is_same_v <T, vec2>)
// 		return thunder::vec2;
// 	if constexpr (std::is_same_v <T, vec3>)
// 		return thunder::vec3;
// 	if constexpr (std::is_same_v <T, vec4>)
// 		return thunder::vec4;

// 	if constexpr (std::is_same_v <T, ivec2>)
// 		return thunder::ivec2;
// 	if constexpr (std::is_same_v <T, ivec3>)
// 		return thunder::ivec3;
// 	if constexpr (std::is_same_v <T, ivec4>)
// 		return thunder::ivec4;

// 	if constexpr (std::is_same_v <T, uvec2>)
// 		return thunder::uvec2;
// 	if constexpr (std::is_same_v <T, uvec3>)
// 		return thunder::uvec3;
// 	if constexpr (std::is_same_v <T, uvec4>)
// 		return thunder::uvec4;

// 	if constexpr (std::is_same_v <T, mat2>)
// 		return thunder::mat2;
// 	if constexpr (std::is_same_v <T, mat3>)
// 		return thunder::mat3;
// 	if constexpr (std::is_same_v <T, mat4>)
// 		return thunder::mat4;

// 	return thunder::bad;
// }

} // namespace jvl::ire
