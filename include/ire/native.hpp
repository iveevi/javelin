#pragma once

#include <type_traits>

#include "../thunder/enumerations.hpp"
#include "emitter.hpp"
#include "tagged.hpp"
#include "type.hpp"

namespace jvl::ire {

// Methods to translate native C++ primitives
thunder::Index translate_primitive(bool);
thunder::Index translate_primitive(int32_t);
thunder::Index translate_primitive(uint32_t);
thunder::Index translate_primitive(uint64_t);
thunder::Index translate_primitive(float);

// Concepts for natives
template <typename T>
concept native = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <thunder::Index>;
};

template <typename T>
concept integral_native = native <T> && std::is_integral_v <T>;

template <typename T>
concept floating_native = native <T> && std::is_floating_point_v <T>;

// TODO: establish semantics for builtin types...
// unless assigning by cache_index_t, every copy/creation is concrete
template <native T>
struct native_t : tagged {
	using tagged::tagged;

	using native_type = T;
	using arithmetic_type = native_t;

	static constexpr thunder::PrimitiveType primitive() {
		if constexpr (std::same_as <T, bool>)
			return thunder::boolean;
		if constexpr (std::same_as <T, int32_t>)
			return thunder::i32;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::u32;
		if constexpr (std::same_as <T, uint64_t>)
			return thunder::u64;
		if constexpr (std::same_as <T, float>)
			return thunder::f32;
		return thunder::bad;
	}

	static constexpr thunder::IntrinsicOperation caster() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::cast_to_int;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::cast_to_uint;
		if constexpr (std::same_as <T, float>)
			return thunder::cast_to_float;

		return thunder::cast_to_int;
	}

	static thunder::Index type() {
		auto &em = Emitter::active;
		return em.emit_type_information(-1, -1, primitive());
	}

	native_t(T v = T()) {
		this->ref = translate_primitive(v);
	}

	// Explicit conversion operations (and copy constructor)
	template <native U>
	requires std::is_convertible_v <U, T>
	explicit native_t(const native_t <U> &other) {
		// Always copy to avoid soft links
		auto &em = Emitter::active;
		thunder::Index args = em.emit_list(other.synthesize().id);
		thunder::Index cast = em.emit_intrinsic(args, caster());
		em.emit_store(synthesize().id, cast);
	}

	// Two-step conversion: U -> native_t <X> -> X
	template <builtin U>
	requires std::is_convertible_v <typename U::arithmetic_type::native_type, T>
	explicit native_t(const U &other) {
		auto &em = Emitter::active;
		thunder::Index args = em.emit_list(other.synthesize().id);
		thunder::Index cast = em.emit_intrinsic(args, caster());
		em.emit_store(synthesize().id, cast);
	}

	native_t &operator=(const T &v) {
		auto &em = Emitter::active;
		if (cached())
			em.emit_store(ref.id, translate_primitive(v));
		else
			ref = translate_primitive(v);

		return *this;
	}

	native_t &operator=(const native_t &v) {
		auto &em = Emitter::active;
		if (cached())
			em.emit_store(ref.id, v.synthesize().id);
		else
			ref = v.synthesize().id;

		return *this;
	}

	cache_index_t synthesize() const {
		if (cached())
			return this->ref;

		this->ref = translate_primitive(T());

		return this->ref;
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

	friend native_t operator+(const native_t &a, const native_t &b) {
		return operation_from_args <native_t> (thunder::addition, a, b);
	}

	friend native_t operator-(const native_t &a, const native_t &b) {
		return operation_from_args <native_t> (thunder::subtraction, a, b);
	}

	friend native_t operator*(const native_t &a, const native_t &b) {
		return operation_from_args <native_t> (thunder::multiplication, a, b);
	}

	friend native_t operator/(const native_t &a, const native_t &b) {
		return operation_from_args <native_t> (thunder::division, a, b);
	}

	// TODO: different implementations for floating point (fmod)
	friend native_t operator%(const native_t &a, const native_t &b) {
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

// TODO: separate header...
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
using arithmetic_base = decltype(underlying(*reinterpret_cast <T *> ((void *) nullptr)));

template <typename T>
concept integral_arithmetic = arithmetic <T> && std::is_integral_v <typename arithmetic_base <T> ::native_type>;

template <typename T>
concept floating_arithmetic = arithmetic <T> && std::is_floating_point_v <typename arithmetic_base <T> ::native_type>;

// Override type generation for natives
template <native T>
struct type_info_generator <native_t <T>> {
	type_info_generator(const native_t <T> &) {}

	intermediate_type synthesize() {
		return primitive_type(native_t <T> ::primitive());
	}
};

} // namespace jvl::ire
