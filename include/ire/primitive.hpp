#pragma once

#include <type_traits>

#include "emitter.hpp"
#include "tagged.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::ire {

// Methods to upcast C++ primitives into a workable type
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
	p.udata = i;

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
concept native = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <thunder::index_t>;
};

template <typename T>
concept integral_native = native <T> && std::is_integral_v <T>;

template <native T>
struct native_t : tagged {
	using tagged::tagged;

	using arithmetic_type = native_t;

	T value;

	native_t(T v = T()) : tagged(), value(v) {}

	native_t operator-() const {
		auto &em = Emitter::active;

		thunder::Operation neg;
		neg.a = synthesize().id;
		neg.code = thunder::OperationCode::unary_negation;

		cache_index_t cit;
		cit = em.emit(neg);
		return cit;
	}

	native_t &operator=(const T &v) {
		auto &em = Emitter::active;
		if (cached()) {
			// At this point we are required to have storage for this
			em.emit_store(ref.id, translate_primitive(v), false);
		} else {
			ref = translate_primitive(v);
		}

		return *this;
	}

	native_t &operator=(const native_t &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		if (cached()) {
			// At this point we are required to have storage for this
			em.emit_store(ref.id, v.synthesize().id, false);
		} else {
			ref = v.synthesize().id;
		}

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

	///////////////////////////////////
	// In place arithmetic operators //
	///////////////////////////////////
	
	native_t &operator+=(const native_t &a) {
		// TODO: store instruction!
		*this = operation_from_args <native_t> (thunder::addition, (*this), a);
		return *this;
	}

	native_t &operator-=(const native_t &a) {
		*this = operation_from_args <native_t> (thunder::subtraction, (*this), a);
		return *this;
	}

	native_t &operator*=(const native_t &a) {
		*this = operation_from_args <native_t> (thunder::multiplication, (*this), a);
		return *this;
	}

	native_t &operator/=(const native_t &a) {
		*this = operation_from_args <native_t> (thunder::division, (*this), a);
		return *this;
	}

	////////////////////////////////
	// In place bitwise operators //
	////////////////////////////////

	template <integral_native U>
	native_t &operator>>=(const native_t <U> &a)
	requires integral_native <T> {
		*this = operation_from_args <native_t> (thunder::bit_shift_right, (*this), a);
		return *this;
	}
	
	template <integral_native U>
	native_t &operator<<=(const native_t <U> &a)
	requires integral_native <T> {
		*this = operation_from_args <native_t> (thunder::bit_shift_left, (*this), a);
		return *this;
	}

	native_t &operator|=(const native_t &a)
	requires integral_native <T> {
		*this = operation_from_args <native_t> (thunder::bit_or, (*this), a);
		return *this;
	}
	
	native_t &operator&=(const native_t &a)
	requires integral_native <T> {
		*this = operation_from_args <native_t> (thunder::bit_and, (*this), a);
		return *this;
	}

	//////////////////////////
	// Arithmetic operators //
	//////////////////////////

	friend native_t operator+(const native_t &a, const native_t &b)
	{
		return operation_from_args <native_t> (thunder::addition, a, b);
	}
	
	friend native_t operator-(const native_t &a, const native_t &b)
	{
		return operation_from_args <native_t> (thunder::subtraction, a, b);
	}
	
	friend native_t operator*(const native_t &a, const native_t &b)
	{
		return operation_from_args <native_t> (thunder::multiplication, a, b);
	}
	
	friend native_t operator/(const native_t &a, const native_t &b)
	{
		return operation_from_args <native_t> (thunder::division, a, b);
	}
	
	// TODO: different implementations for floating point (fmod)
	friend native_t operator%(const native_t &a, const native_t &b)
	{
		return operation_from_args <native_t> (thunder::modulus, a, b);
	}
	
	///////////////////////
	// Bitwise operators //
	///////////////////////

	// Only for integral types
	friend native_t operator|(const native_t &a, const native_t &b)
	requires integral_native <T> {
		return operation_from_args <native_t> (thunder::bit_or, a, b);
	}
	
	friend native_t operator&(const native_t &a, const native_t &b)
	requires integral_native <T> {
		return operation_from_args <native_t> (thunder::bit_and, a, b);
	}
	
	friend native_t operator^(const native_t &a, const native_t &b)
	requires integral_native <T> {
		return operation_from_args <native_t> (thunder::bit_xor, a, b);
	}

	template <integral_native U>
	friend native_t operator>>(const native_t &a, const native_t <U> &b)
	requires integral_native <T> {
		return operation_from_args <native_t> (thunder::bit_shift_right, a, b);
	}

	template <integral_native U>
	friend native_t operator<<(const native_t &a, const native_t <U> &b)
	requires integral_native <T> {
		return operation_from_args <native_t> (thunder::bit_shift_left, a, b);
	}

	///////////////////////
	// Logical operators //
	///////////////////////

	friend native_t operator||(const native_t &a, const native_t &b)
	requires std::same_as <T, bool> {
		return operation_from_args <native_t> (thunder::bool_or, a, b);
	}

	friend native_t operator&&(const native_t &a, const native_t &b)
	requires std::same_as <T, bool> {
		return operation_from_args <native_t> (thunder::bool_and, a, b);
	}

	//////////////////////////
	// Comparison operators //
	//////////////////////////

	using bool_t = native_t <bool>;

	friend bool_t operator==(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::equals, a, b);
	}

	friend bool_t operator!=(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::not_equals, a, b);
	}

	friend bool_t operator>(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_ge, a, b);
	}

	friend bool_t operator<(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_le, a, b);
	}

	friend bool_t operator>=(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_geq, a, b);
	}

	friend bool_t operator<=(const native_t &a, const native_t &b) {
		return operation_from_args <bool_t> (thunder::cmp_leq, a, b);
	}
};

} // namespace jvl::ire
