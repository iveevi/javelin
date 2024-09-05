#pragma once

#include "emitter.hpp"
#include "tagged.hpp"
#include "thunder/enumerations.hpp"
#include <type_traits>

namespace jvl::ire {

// Way to upcast C++ primitives into a workable type
inline thunder::index_t translate_primitive(bool b)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::boolean;
	p.idata = b;

	return em.emit(p);
}

inline thunder::index_t translate_primitive(int32_t i)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::i32;
	p.idata = i;

	return em.emit(p);
}

inline thunder::index_t translate_primitive(uint32_t i)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::u32;
	p.idata = i;

	return em.emit(p);
}

inline thunder::index_t translate_primitive(float f)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::f32;
	p.fdata = f;

	return em.emit(p);
}

template <typename T>
concept primitive_type = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <thunder::index_t>;
};

template <primitive_type T>
struct primitive_t : tagged {
	using tagged::tagged;

	T value;

	primitive_t(T v = T()) : tagged(), value(v) {}

	primitive_t operator-() const {
		auto &em = Emitter::active;

		thunder::List list;
		list.item = synthesize().id;

		thunder::Operation neg;
		neg.args = em.emit(list);
		neg.code = thunder::OperationCode::unary_negation;

		cache_index_t cit;
		cit = em.emit(neg);
		return cit;
	}

	primitive_t &operator=(const T &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		thunder::Store store;
		store.dst = ref.id;
		store.src = translate_primitive(v);
		em.emit_main(store);

		return *this;
	}

	primitive_t &operator=(const primitive_t &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		thunder::Store store;
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

	void refresh(const cache_index_t::value_type &ci) {
		ref = ci;
	}

	// In place arithmetic operators
	primitive_t &operator+=(const primitive_t &a) {
		// TODO: store instruction!
		*this = operation_from_args <primitive_t> (thunder::addition, (*this), a);
		return *this;
	}

	primitive_t &operator-=(const primitive_t &a) {
		*this = operation_from_args <primitive_t> (thunder::subtraction, (*this), a);
		return *this;
	}

	primitive_t &operator*=(const primitive_t &a) {
		*this = operation_from_args <primitive_t> (thunder::multiplication, (*this), a);
		return *this;
	}

	primitive_t &operator/=(const primitive_t &a) {
		*this = operation_from_args <primitive_t> (thunder::division, (*this), a);
		return *this;
	}

	// Bitwise operators
	friend primitive_t operator&(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bit_and, a, b);
	}
	
	friend primitive_t operator|(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bit_or, a, b);
	}
	
	// Bitwise operators
	friend primitive_t operator>>(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bit_shift_right, a, b);
	}
	
	friend primitive_t operator<<(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bit_shift_left, a, b);
	}

	// Logical operators
	// TODO: only for bool types
	friend primitive_t operator||(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bool_or, a, b);
	}

	friend primitive_t operator&&(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <primitive_t> (thunder::bool_and, a, b);
	}

	// Comparison operators
	using bool_t = primitive_t <bool>;

	friend bool_t operator==(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::equals, a, b);
	}

	friend bool_t operator!=(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::not_equals, a, b);
	}

	friend bool_t operator>(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_ge, a, b);
	}

	friend bool_t operator<(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_le, a, b);
	}

	friend bool_t operator>=(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_geq, a, b);
	}

	friend bool_t operator<=(const primitive_t &a, const primitive_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_leq, a, b);
	}
};

} // namespace jvl::ire
