#pragma once

#include "native.hpp"

namespace jvl::ire {

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