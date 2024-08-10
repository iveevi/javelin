#pragma once

#include "../vector.hpp"
#include "../util.hpp"

namespace jvl::ire {

// Shader intrinsic types
struct __gl_Position_t {
	using self = __gl_Position_t;

	swizzle_element <float, self, atom::Swizzle::x> x;
	swizzle_element <float, self, atom::Swizzle::y> y;
	swizzle_element <float, self, atom::Swizzle::z> z;
	swizzle_element <float, self, atom::Swizzle::w> w;

	__gl_Position_t() : x(this), y(this), z(this), w(this) {
		auto &em = Emitter::active;

		// Since there is only going to be one instance...
		em.persist_cache_index(x.ref);
		em.persist_cache_index(y.ref);
		em.persist_cache_index(z.ref);
		em.persist_cache_index(w.ref);
	}

	const __gl_Position_t &operator=(const vec <float, 4> &other) const {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = atom::Global::glsl_vertex_intrinsic_gl_Position;

		atom::Store store;
		store.dst = em.emit(global);
		store.src = other.synthesize().id;

		em.emit_main(store);

		return *this;
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <vec <float, 4>> ().id;
		global.qualifier = atom::Global::glsl_vertex_intrinsic_gl_Position;

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

template <typename T>
vec <T, 3> cross(const vec <T, 3> &v, const vec <T, 3> &w)
{
	return platform_intrinsic_from_args <vec <T, 3>> ("cross", v, w);
}

template <typename T, size_t N>
vec <T, N> normalize(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("normalize", v);
}

} // namespace jvl::ire
