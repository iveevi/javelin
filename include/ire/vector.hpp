#pragma once

#include "../thunder/enumerations.hpp"
#include "native.hpp"
#include "swizzle_base.hpp"
#include "util.hpp"

namespace jvl::ire {

///////////////////////////////////////
// Final vector type; implementation //
///////////////////////////////////////

template <native T, size_t N>
struct vec : swizzle_base <T, N> {
	using swizzle_base <T, N> ::swizzle_base;

	using native_type = T;
	using arithmetic_type = vec <T, N>;

	vec &operator=(const vec &other) {
		auto &em = Emitter::active;

		thunder::Store store;
		store.dst = this->synthesize().id;
		store.src = other.synthesize().id;

		em.emit(store);

		return *this;
	}

	// In place arithmetic operators
	using scalar = native_t <T>;

	vec &operator+=(const vec &a) {
		*this = operation_from_args <vec> (thunder::addition, (*this), a);
		return *this;
	}

	vec &operator-=(const vec &a) {
		*this = operation_from_args <vec> (thunder::subtraction, (*this), a);
		return *this;
	}

	vec &operator*=(const vec &a) {
		*this = operation_from_args <vec> (thunder::multiplication, (*this), a);
		return *this;
	}

	vec &operator*=(const scalar &a) {
		*this = operation_from_args <vec> (thunder::multiplication, (*this), a);
		return *this;
	}

	vec &operator/=(const vec &a) {
		*this = operation_from_args <vec> (thunder::division, (*this), a);
		return *this;
	}

	vec &operator^=(const vec &a) {
		*this = operation_from_args <vec> (thunder::bit_xor, (*this), a);
		return *this;
	}

	// Arithmetic operators
	vec operator-() const {
		return operation_from_args <vec> (thunder::unary_negation, *this);
	}

	friend vec operator+(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::addition, a, b);
	}

	friend vec operator-(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::subtraction, a, b);
	}

	friend vec operator/(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::division, a, b);
	}

	friend vec operator*(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::multiplication, a, b);
	}

	// Mixed arithmetic operators
	#define MIXED_OP(sym, code)							\
		friend vec operator sym(const vec &a, const native_t <T> &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}									\
		friend vec operator sym(const native_t <T> &a, const vec &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}

	MIXED_OP(+, addition);
	MIXED_OP(-, subtraction);
	MIXED_OP(*, multiplication);
	MIXED_OP(/, division);

	// Mixed arithmetic operators with C++ primitives
	#define MIXED_PRIMITIVE_OP(sym, code)						\
		friend vec operator sym(const vec &a, const T &b) {			\
			return a sym native_t <T> (b);					\
		}									\
		friend vec operator sym(const T &a, vec &b) {				\
			return native_t <T> (a) sym b;					\
		}

	MIXED_PRIMITIVE_OP(+, addition);
	MIXED_PRIMITIVE_OP(-, subtraction);
	MIXED_PRIMITIVE_OP(*, multiplication);
	MIXED_PRIMITIVE_OP(/, division);

	// Modulus operator
	friend vec operator%(const vec &a, const native_t <T> &b) {
		return operation_from_args <vec> (thunder::modulus, a, b);
	}

	// Common intrinsic functions
	friend vec fract(const vec &a) {
		return platform_intrinsic_from_args <vec> (thunder::fract, a);
	}

	friend vec reflect(const vec &a, const vec &b) {
		return platform_intrinsic_from_args <vec> (thunder::reflect, a, b);
	}
};

// Vector type concept
template <typename T>
struct is_vector_base : std::false_type {};

template <native T, size_t N>
struct is_vector_base <vec <T, N>> : std::true_type {};

template <typename T>
concept vector_arithmetic = is_vector_base <typename T::arithmetic_type> ::value;

// Bitwise operators
template <integral_native T, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_or, a, b);
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const U &b)
{
	return a | vec <T, N> (b);
}

template <integral_native T, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_and, a, b);
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const U &b)
{
	return a & vec <T, N> (b);
}

// Shifting operators
template <integral_native T, integral_native U, size_t N>
vec <T, N> operator>>(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_right, a, native_t <U> (b));
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator<<(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_left, a, native_t <U> (b));
}

// Override type generation for vectors
template <native T, size_t D>
struct type_info_generator <vec <T, D>> {
	type_info_generator(const vec <T, D> &) {}

	intermediate_type synthesize() {
		return primitive_type(vec <T, D> ::primitive());
	}
};

} // namespace jvl::ire