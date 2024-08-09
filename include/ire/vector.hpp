#pragma once

#include "ire/type_synthesis.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "util.hpp"

namespace jvl::ire {

// Vector types
template <primitive_type T, size_t N>
struct vec;

template <primitive_type T, size_t N>
requires (N >= 1 && N <= 4)
struct swizzle_base : tagged {};

// Other intrinsics
struct __gl_Position_t;

// Swizzle element
template <primitive_type T, typename Up, atom::Swizzle::Kind swz>
class swizzle_element : tagged {
	Up *upper;

	swizzle_element(Up *upper_) : upper(upper_) {}
public:
	operator primitive_t <T> () const {
		return primitive_t <T> (synthesize());
	}

	cache_index_t synthesize() const {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		atom::Swizzle swizzle;
		swizzle.type = swz;
		swizzle.src = upper->synthesize().id;

		em.mark_used(swizzle.src, true);

		return (this->ref = em.emit(swizzle));
	}

	swizzle_element &operator=(const primitive_t <T> &v) {
		auto &em = Emitter::active;

		atom::Store store;
		store.src = v.synthesize().id;
		store.dst = synthesize().id;

		em.emit_main(store);

		return *this;
	}

	primitive_t <T> operator-() const {
		return primitive_t <T> (synthesize()).operator-();
	}

	template <primitive_type U, size_t N>
	requires (N >= 1 && N <= 4)
	friend class swizzle_base;

	friend struct __gl_Position_t;
};

// Swizzle base specializations
template <primitive_type T>
class swizzle_base <T, 2> : public tagged {
	using self = swizzle_base <T, 2>;

	T initial[2];
public:
	swizzle_element <T, self, atom::Swizzle::x> x;
	swizzle_element <T, self, atom::Swizzle::y> y;

	swizzle_base(T x = T(0), T y = T(0))
			: x(this), y(this) {
		initial[0] = x;
		initial[1] = y;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = synthesize_type_fields <vec <T, 2>> ().id;
		ctor.args = synthesize_list(initial[0], initial[1]);

		return (ref = em.emit(ctor));
	}
};

template <primitive_type T>
class swizzle_base <T, 3> : public tagged {
	using self = swizzle_base <T, 3>;

	T initial[3];
public:
	swizzle_element <T, self, atom::Swizzle::x> x;
	swizzle_element <T, self, atom::Swizzle::y> y;
	swizzle_element <T, self, atom::Swizzle::z> z;

	swizzle_base(T x = T(0), T y = T(0), T z = T(0))
			: x(this), y(this), z(this) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;
		atom::Construct ctor;
		ctor.type = synthesize_type_fields <vec <T, 3>> ().id;
		ctor.args = synthesize_list(initial[0], initial[1], initial[2]);

		return (ref = em.emit(ctor));
	}
};

template <primitive_type T>
class swizzle_base <T, 4> : public tagged {
	using self = swizzle_base <T, 4>;

	T initial[4];
public:
	swizzle_element <T, self, atom::Swizzle::x> x;
	swizzle_element <T, self, atom::Swizzle::y> y;
	swizzle_element <T, self, atom::Swizzle::z> z;
	swizzle_element <T, self, atom::Swizzle::w> w;

	swizzle_base(T x = T(0), T y = T(0), T z = T(0), T w = T(0))
			: x(this), y(this), z(this), w(this) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
		initial[3] = w;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = synthesize_type_fields <vec <T, 4>> ().id;
		ctor.args = synthesize_list(initial[0], initial[1], initial[2], initial[3]);

		return (ref = em.emit(ctor));
	}
};

// Final vector type
template <primitive_type T, size_t N>
struct vec : swizzle_base <T, N> {
	using swizzle_base <T, N> ::swizzle_base;

	vec &operator=(const vec &other) {

		auto &em = Emitter::active;

		atom::Store store;
		store.dst = this->synthesize().id;
		store.src = other.synthesize().id;

		em.emit_main(store);

		return *this;
	}
};

// Shader intrinsic types
struct __gl_Position_t {
	using self = __gl_Position_t;

	// TODO: macros to include all swizzles
	// TODO: zero storage swizzle members
	swizzle_element <float, self, atom::Swizzle::x> x;
	swizzle_element <float, self, atom::Swizzle::y> y;
	swizzle_element <float, self, atom::Swizzle::z> z;
	swizzle_element <float, self, atom::Swizzle::w> w;

	__gl_Position_t() : x(this), y(this), z(this), w(this) {}

	const __gl_Position_t &operator=(const vec <float, 4> &other) const {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = synthesize_type_fields <vec <float, 4>> ().id;
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
		global.type = synthesize_type_fields <vec <float, 4>> ().id;
		global.qualifier = atom::Global::glsl_vertex_intrinsic_gl_Position;

		cache_index_t cit;
		return (cit = em.emit_main(global));
	}
} static gl_Position;

} // namespace jvl::ire
