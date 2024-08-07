#pragma once

#include "gltype.hpp"
#include "tagged.hpp"
#include "util.hpp"

namespace jvl::ire {

// Vector types
template <gltype_complatible T, size_t N>
struct vec;

template <gltype_complatible T, size_t N>
requires (N >= 1 && N <= 4)
struct swizzle_base : tagged {};

template <gltype_complatible T, typename Up, op::Swizzle::Kind swz>
class swizzle_element : tagged {
	Up *upper;

	swizzle_element(Up *upper_) : upper(upper_) {}
public:
	operator gltype <T> () const {
		return gltype <T> (synthesize());
	}

	cache_index_t synthesize() const {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		op::Swizzle swizzle;
		swizzle.type = swz;
		swizzle.src = upper->synthesize().id;

		em.mark_used(swizzle.src, true);

		return (this->ref = em.emit(swizzle));
	}

	template <gltype_complatible U, size_t N>
	requires (N >= 1 && N <= 4)
	friend class swizzle_base;
};

template <gltype_complatible T>
class swizzle_base <T, 2> : public tagged {
	using self = swizzle_base <T, 2>;

	T initial[2];
public:
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;

	swizzle_base(T x = T(0), T y = T(0))
			: x(this), y(this) {
		initial[0] = x;
		initial[1] = y;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 2>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = synthesize_list(initial[0], initial[1]);

		return (ref = em.emit(ctor));
	}
};

template <gltype_complatible T>
class swizzle_base <T, 3> : public tagged {
	using self = swizzle_base <T, 3>;

	T initial[3];
public:
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;
	swizzle_element <T, self, op::Swizzle::z> z;

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

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 3>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = synthesize_list(initial[0], initial[1], initial[2]);

		return (ref = em.emit(ctor));
	}
};

template <gltype_complatible T>
class swizzle_base <T, 4> : public tagged {
	using self = swizzle_base <T, 4>;

	T initial[4];
public:
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;
	swizzle_element <T, self, op::Swizzle::z> z;
	swizzle_element <T, self, op::Swizzle::w> w;

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

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 4>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = synthesize_list(initial[0], initial[1], initial[2], initial[3]);

		return (ref = em.emit(ctor));
	}
};

template <gltype_complatible T, size_t N>
struct vec : swizzle_base <T, N> {
	using swizzle_base <T, N> ::swizzle_base;
};

} // namespace jvl::ire
