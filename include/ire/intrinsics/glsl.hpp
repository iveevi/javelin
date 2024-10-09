#pragma once

#include "../vector.hpp"
#include "../util.hpp"
#include "ire/tagged.hpp"
#include "ire/type_synthesis.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::ire {

// Immutable instrinsic types
template <generic, thunder::QualifierKind>
struct __glsl_intrinsic_variable_t {};

// Implementation for native types
template <native T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <T, code> {
	using arithmetic_type = native_t <T>;

	__glsl_intrinsic_variable_t() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t intr = em.emit_qualifier(type, -1, code);
		return cache_index_t::from(intr);
	}

	operator arithmetic_type() const {
		return synthesize();
	}
};

// Implementation for vector types
template <native T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <vec <T, 4>, code> {
	using arithmetic_type = vec <T, 4>;
	
	using self = __glsl_intrinsic_variable_t;
	
	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;
	swizzle_element <T, self, thunder::SwizzleCode::z> z;
	swizzle_element <T, self, thunder::SwizzleCode::w> w;

	__glsl_intrinsic_variable_t() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <vec <T, 4>> ().id;
		thunder::index_t intr = em.emit_qualifier(type, -1, code);
		return cache_index_t::from(intr);
	}

	operator arithmetic_type() const {
		return synthesize();
	}
	
	// Assignment only for non-const intrinsics (assignable)
	const self &operator=(const vec <T, 4> &other) {
		auto &em = Emitter::active;
		em.emit_store(synthesize().id, other.synthesize().id);
		return *this;
	}
};

template <thunder::QualifierKind code>
using __glsl_int = __glsl_intrinsic_variable_t <int32_t, code>;

template <thunder::QualifierKind code>
using __glsl_float = __glsl_intrinsic_variable_t <float, code>;

template <thunder::QualifierKind code>
using __glsl_vec4 = __glsl_intrinsic_variable_t <vec <float, 4>, code>;

////////////////////////////////////////////////
// GLSL shader intrinsic variable definitions //
////////////////////////////////////////////////

static const __glsl_vec4  <thunder::glsl_intrinsic_gl_FragCoord>   gl_FragCoord;
static const __glsl_float <thunder::glsl_intrinsic_gl_FragDepth>   gl_FragDepth;
static const __glsl_int   <thunder::glsl_intrinsic_gl_VertexID>    gl_VertexID;
static const __glsl_int   <thunder::glsl_intrinsic_gl_VertexIndex> gl_VertexIndex;

static       __glsl_vec4 <thunder::glsl_intrinsic_gl_Position>     gl_Position;

/////////////////////////////////////
// GLSL shader intrinsic functions //
/////////////////////////////////////

// Operation instrinsics
template <floating_arithmetic A, floating_arithmetic B>
requires (std::same_as <arithmetic_base <A>, arithmetic_base <B>>)
auto mod(const A &x, const B &y)
{
	using result = native_t <typename arithmetic_base <A> ::native_type>;
	return platform_intrinsic_from_args <result> (thunder::mod, underlying(x), underlying(y));
}

template <floating_native A, floating_arithmetic B, size_t N>
requires (std::same_as <A, typename arithmetic_base <B> ::native_type>)
auto mod(const vec <A, N> &x, const B &y)
{
	return platform_intrinsic_from_args <vec <A, N>> (thunder::mod, underlying(x), underlying(y));
}

// Derivative intrinsics
template <native T, size_t N>
vec <T, N> dFdx(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdx, v);
}

template <native T, size_t N>
vec <T, N> dFdy(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdy, v);
}

template <native T, size_t N>
vec <T, N> dFdxFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdxFine, v);
}

template <native T, size_t N>
vec <T, N> dFdyFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdyFine, v);
}

// Casting intrinsics for scalar types
inline native_t <float> intBitsToFloat(const native_t <int32_t> &v)
{
	return platform_intrinsic_from_args <native_t <float>> (thunder::glsl_intBitsToFloat, v);
}

// Casting intrinsics for vector types
inline native_t <int32_t> floatBitsToInt(const native_t <float> &v)
{
	return platform_intrinsic_from_args <native_t <int32_t>> (thunder::glsl_floatBitsToInt, v);
}

inline native_t <uint32_t> floatBitsToUint(const native_t <float> &v)
{
	return platform_intrinsic_from_args <native_t <uint32_t>> (thunder::glsl_floatBitsToUint, v);
}

template <size_t N>
vec <int32_t, N> floatBitsToInt(const vec <float, N> &v)
{
	return platform_intrinsic_from_args <vec <int32_t, N>> (thunder::glsl_floatBitsToInt, v);
}

template <size_t N>
vec <uint32_t, N> floatBitsToUint(const vec <float, N> &v)
{
	return platform_intrinsic_from_args <vec <uint32_t, N>> (thunder::glsl_floatBitsToUint, v);
}

template <size_t N>
vec <float, N> intBitsToFloat(const vec <int32_t, N> &v)
{
	return platform_intrinsic_from_args <vec <float, N>> (thunder::glsl_intBitsToFloat, v);
}

template <size_t N>
vec <float, N> uintBitsToFloat(const vec <uint32_t, N> &v)
{
	return platform_intrinsic_from_args <vec <float, N>> (thunder::glsl_uintBitsToFloat, v);
}

} // namespace jvl::ire
