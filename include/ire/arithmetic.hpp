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
auto operator+(const A &a, const B &b)
{
	return underlying(a) + underlying(b);
}

// Subtraction
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
auto operator-(const A &a, const B &b)
{
	return underlying(a) - underlying(b);
}

// Multiplication
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
auto operator*(const A &a, const B &b)
{
	return underlying(a) * underlying(b);
}

// Division
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
auto operator/(const A &a, const B &b)
{
	return underlying(a) / underlying(b);
}

// Modulus
template <integral_arithmetic A, integral_arithmetic B>
requires overload_compatible <A, B>
auto operator%(const A &a, const B &b)
{
	return underlying(a) % underlying(b);
}

// Bit OR
template <integral_arithmetic A, integral_arithmetic B>
requires overload_compatible <A, B>
auto operator|(const A &a, const B &b)
{
	return underlying(a) | underlying(b);
}

// Bit AND
template <integral_arithmetic A, integral_arithmetic B>
requires overload_compatible <A, B>
auto operator&(const A &a, const B &b)
{
	return underlying(a) & underlying(b);
}

// Bit XOR
template <integral_arithmetic A, integral_arithmetic B>
requires overload_compatible <A, B>
auto operator^(const A &a, const B &b)
{
	return underlying(a) ^ underlying(b);
}

// Logical comparison
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
native_t <bool> operator==(const A &a, const B &b)
{
	return underlying(a) == underlying(b);
}

template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
native_t <bool> operator!=(const A &a, const B &b)
{
	return underlying(a) != underlying(b);
}

template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
native_t <bool> operator>(const A &a, const B &b)
{
	return underlying(a) > underlying(b);
}

template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
native_t <bool> operator<(const A &a, const B &b)
{
	return underlying(a) < underlying(b);
}

// Shifting operators
template <integral_arithmetic A, integral_arithmetic B>
requires integral_native <typename decltype(underlying(B()))::native_type>
auto operator<<(const A &a, const B &b)
{
	return underlying(a) << underlying(b);
}

template <integral_arithmetic A, integral_arithmetic B>
requires integral_native <typename decltype(underlying(B()))::native_type>
auto operator>>(const A &a, const B &b)
{
	return underlying(a) >> underlying(b);
}

} // namespace jvl::ire
