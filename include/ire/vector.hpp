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

	swizzle_base(cache_index_t cit)
			: tagged(cit), x(this), y(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 2>> ().id;
		ctor.args = list_from_args(initial[0], initial[1]);

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

	swizzle_base(const primitive_t <T> &x, const primitive_t <T> &y, const primitive_t <T> &z) : swizzle_base() {
		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 4>> ().id;
		ctor.args = list_from_args(x, y, z);

		ref = em.emit(ctor);
	}

	swizzle_base(cache_index_t cit)
			: tagged(cit), x(this), y(this), z(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;
		atom::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 3>> ().id;
		ctor.args = list_from_args(initial[0], initial[1], initial[2]);

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

	swizzle_base(const swizzle_base <T, 3> &v, const primitive_t <T> &x) : swizzle_base() {
		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 4>> ().id;
		ctor.args = list_from_args(v, x);

		ref = em.emit(ctor);
	}

	swizzle_base(cache_index_t cit)
			: tagged(cit), x(this), y(this), z(this), w(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		atom::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 4>> ().id;
		ctor.args = list_from_args(initial[0], initial[1], initial[2], initial[3]);

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

} // namespace jvl::ire
