#pragma once

#include "../vector.hpp"
#include "../util.hpp"

namespace jvl::ire {

// Shader intrinsic types
struct __gl_Position_t {
	using self = __gl_Position_t;

	swizzle_element <float, self, thunder::SwizzleCode::x> x;
	swizzle_element <float, self, thunder::SwizzleCode::y> y;
	swizzle_element <float, self, thunder::SwizzleCode::z> z;
	swizzle_element <float, self, thunder::SwizzleCode::w> w;

	__gl_Position_t() : x(this), y(this), z(this), w(this) {}

	const __gl_Position_t &operator=(const vec <float, 4> &other) const {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = thunder::GlobalQualifier::glsl_vertex_intrinsic_gl_Position;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = other.synthesize().id;

		em.emit_main(store);

		return *this;
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = thunder::GlobalQualifier::glsl_vertex_intrinsic_gl_Position;

		cache_index_t cit;
		return (cit = em.emit_main(global));
	}
} static gl_Position;

// Shader intrinsic functions
template <typename T, size_t N>
vec <T, N> dFdx(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("dFdx", v);
}

template <typename T, size_t N>
vec <T, N> dFdy(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("dFdy", v);
}

template <typename T, size_t N>
vec <T, N> dFdxFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("dFdxFine", v);
}

template <typename T, size_t N>
vec <T, N> dFdyFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("dFdyFine", v);
}

} // namespace jvl::ire
