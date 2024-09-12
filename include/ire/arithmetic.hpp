#pragma once

#include "primitive.hpp"
#include "thunder/enumerations.hpp"
#include "vector.hpp"

namespace jvl::ire {

/////////////////////////
// Primitive operators //
/////////////////////////

// Addition
// TODO: macros to avoid the headers
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::addition, a, primitive_t <T> (b));
}

template <native T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const T &a, const U &b)
{
	return primitive_t <T> (a) + b;
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator+(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) + b;
}

// Subtraction
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::subtraction, a, primitive_t <T> (b));
}

template <native T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const T &a, const U &b)
{
	return primitive_t <T> (a) - b;
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator-(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) - b;
}

// Multiplication
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::multiplication, a, primitive_t <T> (b));
}

template <native T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const T &a, const U &b)
{
	return primitive_t <T> (a) * b;
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator*(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) * b;
}

// Division
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::division, a, primitive_t <T> (b));
}

template <native T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const T &a, const U &b)
{
	return primitive_t <T> (a) / b;
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator/(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) / b;
}

// Modulus
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator%(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::modulus, a, primitive_t <T> (b));
}

template <native T, non_trivial_generic U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator%(const T &a, const U &b)
{
	return primitive_t <T> (a) % b;
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator%(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) % b;
}

// Binary OR
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator|(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::bit_or, a, primitive_t <T> (b));
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator|(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) | b;
}

template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator&(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::bit_and, a, primitive_t <T> (b));
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator&(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) & b;
}

// Binary XOR
template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator^(const primitive_t <T> &a, const U &b)
{
	return operation_from_args <primitive_t <T>> (thunder::bit_xor, a, b);
}

template <native T, typename Up, thunder::SwizzleCode swz, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> operator^(const swizzle_element <T, Up, swz> &a, const U &b)
{
	return primitive_t <T> (a) ^ b;
}

// Bit shift operators
template <integral_native T, integral_native U>
primitive_t <T> operator>>(const primitive_t <T> &a, const primitive_t <U> &b)
{
	return operation_from_args <primitive_t <T>> (thunder::bit_shift_right, a, b);
}

template <integral_native T, integral_native U>
primitive_t <T> operator<<(const primitive_t <T> &a, const primitive_t <U> &b)
{
	return operation_from_args <primitive_t <T>> (thunder::bit_shift_left, a, b);
}

} // namespace jvl::ire