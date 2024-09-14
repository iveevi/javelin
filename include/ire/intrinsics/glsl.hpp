#pragma once

#include "../vector.hpp"
#include "../util.hpp"
#include "ire/tagged.hpp"
#include "ire/type_synthesis.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::ire {

// Shader intrinsic types
struct __gl_Position_t {
	using self = __gl_Position_t;

	swizzle_element <float, self, thunder::SwizzleCode::x> x;
	swizzle_element <float, self, thunder::SwizzleCode::y> y;
	swizzle_element <float, self, thunder::SwizzleCode::z> z;
	swizzle_element <float, self, thunder::SwizzleCode::w> w;

	__gl_Position_t() : x(this), y(this), z(this), w(this) {}

	const __gl_Position_t &operator=(const vec <float, 4> &other) {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <vec <float, 4>> ().id;
		global.kind = thunder::glsl_intrinsic_gl_Position;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = other.synthesize().id;

		em.emit(store);

		return *this;
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Qualifier qualifier;
		qualifier.underlying = type_field_from_args <vec <float, 4>> ().id;
		qualifier.kind = thunder::glsl_intrinsic_gl_Position;

		return cache_index_t::from(em.emit(qualifier));
	}
} static gl_Position;

struct __gl_int32_intrinsic {
	using arithmetic_type = native_t <int32_t>;

	thunder::QualifierKind code;

	__gl_int32_intrinsic() = default;
	__gl_int32_intrinsic(thunder::QualifierKind code_) : code(code_) {}

	operator arithmetic_type() const {
		return synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Qualifier qualifier;
		
		qualifier.underlying = type_field_from_args <int32_t> ().id;
		qualifier.kind = code;

		return cache_index_t::from(em.emit(qualifier));
	}
} static // Integral GLSL shader inrinsic variables...
	gl_VertexID(thunder::glsl_intrinsic_gl_VertexID),
	gl_VertexIndex(thunder::glsl_intrinsic_gl_VertexIndex);

// Shader intrinsic functions
template <typename T, size_t N>
vec <T, N> dFdx(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdx, v);
}

template <typename T, size_t N>
vec <T, N> dFdy(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdy, v);
}

template <typename T, size_t N>
vec <T, N> dFdxFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdxFine, v);
}

template <typename T, size_t N>
vec <T, N> dFdyFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdyFine, v);
}

// Casting intrinsics
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
