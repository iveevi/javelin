#pragma once

#include "gltype.hpp"

namespace jvl::ire {

// Vector types
template <typename T, size_t N>
struct vec;

template <gltype_complatible T, typename Up, op::Swizzle::Kind swz>
struct swizzle_element : tagged {
	operator gltype <T> () const {
		return gltype <T> (synthesize());
	}

	cache_index_t synthesize() const {
		if (this->cached())
			return this->ref;

		synthesizable auto &up = *(reinterpret_cast <const Up *> (this));

		auto &em = Emitter::active;

		op::Swizzle swizzle;
		swizzle.type = swz;
		swizzle.src = up.synthesize().id;

		em.mark_used(swizzle.src, true);

		return (this->ref = em.emit(swizzle));
	}
};

template <gltype_complatible T, size_t N>
requires (N >= 1 && N <= 4)
struct swizzle_base : tagged {};

template <gltype_complatible T>
struct swizzle_base <T, 2> : tagged {
	using self = swizzle_base <T, 2>;
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;

	// TODO: private
	T initial[2];
	swizzle_base(T x = T(0), T y = T(0)) {
		initial[0] = x;
		initial[1] = y;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: list method
		op::List a2;
		a2.item = translate_primitive(initial[1]);

		op::List a1;
		a1.item = translate_primitive(initial[0]);
		a1.next = em.emit(a2);

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 2>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(a1);

		return (ref = em.emit(ctor));
	}
};

template <gltype_complatible T>
struct swizzle_base <T, 3> : tagged {
	using self = swizzle_base <T, 3>;
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;
	swizzle_element <T, self, op::Swizzle::z> z;

	// TODO: private
	T initial[3];
	swizzle_base(T x = T(0), T y = T(0), T z = T(0)) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: list method
		op::List a3;
		a3.item = translate_primitive(initial[0]);

		op::List a2;
		a2.item = translate_primitive(initial[1]);
		a2.next = em.emit(a3);

		op::List a1;
		a1.item = translate_primitive(initial[2]);
		a1.next = em.emit(a2);

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 3>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(a1);

		return (ref = em.emit(ctor));
	}
};

template <gltype_complatible T>
struct swizzle_base <T, 4> : tagged {
	using self = swizzle_base <T, 4>;
	swizzle_element <T, self, op::Swizzle::x> x;
	swizzle_element <T, self, op::Swizzle::y> y;
	swizzle_element <T, self, op::Swizzle::z> z;
	swizzle_element <T, self, op::Swizzle::w> w;

	// TODO: private...
	T initial[4];
	swizzle_base(T x = T(0), T y = T(0), T z = T(0), T w = T(0)) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
		initial[3] = w;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: list method
		op::List a4;
		a4.item = translate_primitive(initial[0]);

		op::List a3;
		a3.item = translate_primitive(initial[1]);
		a3.next = em.emit(a4);

		op::List a2;
		a2.item = translate_primitive(initial[2]);
		a2.next = em.emit(a3);

		op::List a1;
		a1.item = translate_primitive(initial[3]);
		a1.next = em.emit(a2);

		// TODO: type field generator
		op::TypeField tf;
		tf.item = primitive_type <vec <T, 4>> ();

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(a1);

		return (ref = em.emit(ctor));
	}
};

template <typename T, size_t N>
struct vec : swizzle_base <T, N> {};

} // namespace jvl::ire
