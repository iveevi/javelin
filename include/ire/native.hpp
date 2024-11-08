#pragma once

#include <type_traits>

#include "emitter.hpp"
#include "tagged.hpp"
#include "../thunder/enumerations.hpp"

namespace jvl::ire {

// Methods to upcast C++ primitives into a workable type
inline thunder::index_t translate_primitive(bool b)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::boolean;
	p.bdata = b;

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

// Generating cast operations
constexpr auto caster(int32_t)
{
	return thunder::cast_to_int;
}

constexpr auto caster(uint32_t)
{
	return thunder::cast_to_uint;
}

constexpr auto caster(float)
{
	return thunder::cast_to_float;
}

// Type class for natives
template <typename T>
concept native = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <thunder::index_t>;
};

template <typename T>
concept integral_native = native <T> && std::is_integral_v <T>;

template <typename T>
concept floating_native = native <T> && std::is_floating_point_v <T>;

template <native T>
struct native_t : tagged {
	using tagged::tagged;

	using native_type = T;
	using arithmetic_type = native_t;

	T value = T();

	native_t(T v = T()) : value(v) {}

	// Explicit conversion operations
	template <native U>
	requires std::is_convertible_v <U, T>
	explicit native_t(const native_t <U> &other) {
		auto &em = Emitter::active;
		if constexpr (std::same_as <T, U>) {
			this->ref = other.ref;
		} else {
			synthesize();
			thunder::index_t args = em.emit_list(other.ref.id);
			thunder::index_t cast = em.emit_intrinsic(args, caster(value));
			em.emit_store(this->ref.id, cast);
		}
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
			em.emit_store(ref.id, v.synthesize().id);
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

	///////////////////////////////////
	// In place arithmetic operators //
	///////////////////////////////////

	native_t operator-() const {
		return operation_from_args <native_t> (thunder::unary_negation, *this);
	}
	
	native_t &operator+=(const native_t &a) {
		// TODO: store instruction?
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

	// Unary negation
	native_t operator!() const
	requires std::is_same_v <T, bool> {
		return operation_from_args <native_t> (thunder::bool_not, *this);
	}

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

///////////////////////////////////////////////
// Type system infrastructure for arithmetic //
///////////////////////////////////////////////

template <typename T>
concept builtin_arithmetic = builtin <T>
		&& builtin <typename T::arithmetic_type>
		&& requires(const T &t) {
	{ typename T::arithmetic_type(t) };
};

template <typename T>
concept arithmetic = native <T> || builtin_arithmetic <T>;

template <native A>
inline auto underlying(const A &a)
{
	return native_t <A> (a);
}

template <builtin_arithmetic A>
inline auto underlying(const A &a)
{
	return typename A::arithmetic_type(a);
}

template <arithmetic T>
using arithmetic_base = decltype(underlying(T()));

template <typename T>
concept integral_arithmetic = arithmetic <T> && std::is_integral_v <typename arithmetic_base <T> ::native_type>;

template <typename T>
concept floating_arithmetic = arithmetic <T> && std::is_floating_point_v <typename arithmetic_base <T> ::native_type>;

} // namespace jvl::ire
