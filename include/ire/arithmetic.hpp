#pragma once

#include "primitive.hpp"
#include "vector.hpp"

namespace jvl::ire {

// Binary arithmetic operators
// General structure, cascade of operators

// Addition
template <primitive_type T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const primitive_t <T> &a, const U &b)
{
	auto pb = primitive_t <T> (b);
	return operation_from_args <primitive_t <T>> (thunder::addition, a, pb);
}

template <primitive_type T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const T &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa + b;
}

template <primitive_type T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const swizzle_element <T, Up, swz> &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa + b;
}

// Subtraction
template <primitive_type T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const primitive_t <T> &a, const U &b)
{
	auto pb = primitive_t <T> (b);
	return operation_from_args <primitive_t <T>> (thunder::subtraction, a, pb);
}

template <primitive_type T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const T &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa - b;
}

template <primitive_type T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const swizzle_element <T, Up, swz> &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa - b;
}

// Multiplication
template <primitive_type T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const primitive_t <T> &a, const U &b)
{
	auto pb = primitive_t <T> (b);
	return operation_from_args <primitive_t <T>> (thunder::multiplication, a, pb);
}

template <primitive_type T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const T &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa * b;
}

template <primitive_type T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const swizzle_element <T, Up, swz> &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa * b;
}

// Division
template <primitive_type T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const primitive_t <T> &a, const U &b)
{
	auto pb = primitive_t <T> (b);
	return operation_from_args <primitive_t <T>> (thunder::division, a, pb);
}

template <primitive_type T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const T &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa / b;
}

template <primitive_type T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const swizzle_element <T, Up, swz> &a, const U &b)
{
	auto pa = primitive_t <T> (a);
	return pa / b;
}

} // namespace jvl::ire