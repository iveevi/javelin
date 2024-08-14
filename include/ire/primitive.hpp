#pragma once

#include "emitter.hpp"
#include "tagged.hpp"
#include <type_traits>

namespace jvl::ire {

// Way to upcast C++ primitives into a workable type
inline int translate_primitive(bool b)
{
	auto &em = Emitter::active;

	atom::Primitive p;
	p.type = atom::boolean;
	p.idata = b;

	return em.emit(p);
}

inline int translate_primitive(int i)
{
	auto &em = Emitter::active;

	atom::Primitive p;
	p.type = atom::i32;
	p.idata = i;

	return em.emit(p);
}

inline int translate_primitive(float f)
{
	auto &em = Emitter::active;

	atom::Primitive p;
	p.type = atom::f32;
	p.fdata = f;

	return em.emit(p);
}

template <typename T>
concept primitive_type = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <int>;
};

template <primitive_type T>
struct primitive_t : tagged {
	using tagged::tagged;

	T value;

	primitive_t(T v = T()) : tagged(), value(v) {
		synthesize();
	}

	primitive_t operator-() const {
		auto &em = Emitter::active;

		atom::List list;
		list.item = synthesize().id;

		atom::Operation neg;
		neg.args = em.emit(list);
		neg.type = atom::Operation::unary_negation;

		cache_index_t cit;
		cit = em.emit(neg);
		return cit;
	}

	primitive_t &operator=(const T &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		atom::Store store;
		store.dst = ref.id;
		store.src = translate_primitive(v);
		em.emit_main(store);

		return *this;
	}

	primitive_t &operator=(const primitive_t &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		atom::Store store;
		store.dst = ref.id;
		store.src = v.synthesize().id;
		em.emit_main(store);

		return *this;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		return (ref = translate_primitive(value));
	}

	// Common intrinsics
	friend primitive_t clamp(const primitive_t &x, const primitive_t &min, const primitive_t &max) {
		return platform_intrinsic_from_args <primitive_t> ("clamp", x, min, max);
	}

	friend primitive_t sqrt(const primitive_t &x)  {
		return platform_intrinsic_from_args <primitive_t> ("sqrt", x);
	}

	friend primitive_t pow(const primitive_t &x, const primitive_t &p) {
		return platform_intrinsic_from_args <primitive_t> ("pow", x, p);
	}

	// Arithmetic operators
	friend primitive_t operator+(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (atom::Operation::addition, a, b);
	}

	friend primitive_t operator-(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (atom::Operation::subtraction, a, b);
	}

	friend primitive_t operator/(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (atom::Operation::division, a, b);
	}

	friend primitive_t operator*(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (atom::Operation::multiplication, a, b);
	}
};

} // namespace jvl::ire
