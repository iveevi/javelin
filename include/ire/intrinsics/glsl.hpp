#pragma once

#include "../vector.hpp"
#include "../util.hpp"
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

	__gl_Position_t() : x(this), y(this), z(this), w(this) {
		auto &em = Emitter::active;
		auto callback = std::bind(&__gl_Position_t::refresh, this);
		em.scoping_callbacks.push_back(callback);
	}

	void refresh() {
		fmt::println("refreshing gl position...");
		x.refresh();
		y.refresh();
		z.refresh();
		w.refresh();
	}

	const __gl_Position_t &operator=(const vec <float, 4> &other) {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = thunder::glsl_vertex_intrinsic_gl_Position;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = other.synthesize().id;

		em.emit_main(store);

		refresh();

		return *this;
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = thunder::glsl_vertex_intrinsic_gl_Position;

		cache_index_t cit;
		return (cit = em.emit_main(global));
	}
} static gl_Position;

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
