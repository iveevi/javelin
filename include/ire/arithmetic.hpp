#pragma once

#include "primitive.hpp"
#include "thunder/enumerations.hpp"
#include "vector.hpp"

namespace jvl::ire {

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

template <builtin_arithmetic A>
inline auto underlying(const A &a)
{
	return typename A::arithmetic_type(a);
}

template <native A>
inline auto underlying(const A &a)
{
	return native_t <A> (a);
}

template <arithmetic T>
using arithmetic_base = decltype(underlying(T()));

// Preventing unwanted overloads
template <typename T, typename U>
concept overload_compatible = true
	&& arithmetic <T>
	&& arithmetic <U>
	&& std::same_as <arithmetic_base <T>, arithmetic_base <U>>
	&& !(native <T> && native <U>);

/////////////////////////
// Primitive operators //
/////////////////////////

// Addition
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator+(const A &a, const B &b)
{
	return underlying(a) + underlying(b);
}

// Subtraction
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator-(const A &a, const B &b)
{
	return underlying(a) - underlying(b);
}

// Multiplication
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator*(const A &a, const B &b)
{
	return underlying(a) * underlying(b);
}

// Division
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator/(const A &a, const B &b)
{
	return underlying(a) / underlying(b);
}

// Modulus
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator%(const A &a, const B &b)
{
	return underlying(a) % underlying(b);
}

// Bit OR
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator|(const A &a, const B &b)
{
	return underlying(a) | underlying(b);
}

// Bit AND
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator&(const A &a, const B &b)
{
	return underlying(a) & underlying(b);
}

// Bit XOR
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
arithmetic_base <A> operator^(const A &a, const B &b)
{
	return underlying(a) ^ underlying(b);
}

} // namespace jvl::ire