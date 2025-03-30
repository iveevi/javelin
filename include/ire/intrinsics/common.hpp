#pragma once

#include "../../thunder/enumerations.hpp"
#include "../native.hpp"
#include "../util.hpp"
#include "../vector.hpp"
#include "../arithmetic.hpp"

namespace jvl::ire {

// Abolsute value
template <arithmetic T>
auto abs(const T &a)
{
	using result = decltype(underlying(a));
	return platform_intrinsic_from_args <result> (thunder::abs, a);
}

// Maximum
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
auto max(const A &a, const B &b)
{
	using R = decltype(underlying(a));
	return platform_intrinsic_from_args <R> (thunder::max, underlying(a), underlying(b));
}

// Minimum
template <arithmetic A, arithmetic B>
requires overload_compatible <A, B>
auto min(const A &a, const B &b)
{
	using R = decltype(underlying(a));
	return platform_intrinsic_from_args <R> (thunder::min, underlying(a), underlying(b));
}

// Square root
template <native T>
native_t <T> sqrt(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::sqrt, x);
}

template <native T, typename Up, thunder::SwizzleCode swz>
native_t <T> sqrt(const swizzle_element <T, Up, swz> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::sqrt, x);
}

// Clamp
template <arithmetic A, arithmetic B, arithmetic C>
requires equivalent <B, C>
auto clamp(const A &x, const B &min, const C &max)
{
	// TODO: check the underlying native types as well...
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::clamp,
		underlying(x),
		underlying(min),
		underlying(max));
}

// Floor
template <floating_arithmetic T>
auto floor(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::floor, x);
}

// Ceiling
template <floating_arithmetic T>
auto ceil(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::ceil, x);
}

// Fractional part
template <floating_arithmetic T>
auto fract(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::fract, x);
}

// Sin
template <floating_arithmetic T>
auto sin(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::sin, underlying(x));
}

// Cos
template <floating_arithmetic T>
auto cos(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::cos, underlying(x));
}

// Tan
template <floating_arithmetic T>
auto tan(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::tan, underlying(x));
}

// Inverse sin
template <floating_arithmetic T>
auto asin(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::asin, underlying(x));
}

// Inverse cos
template <floating_arithmetic T>
auto acos(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::acos, underlying(x));
}

// Inverse tan
template <floating_arithmetic T>
auto atan(const T &x)
{
	return platform_intrinsic_from_args <arithmetic_base <T>> (thunder::atan, x);
}

template <floating_arithmetic T, floating_arithmetic U>
auto atan(const T &x, const U &y)
{
	return platform_intrinsic_from_args <arithmetic_base <T>> (thunder::atan, x, y);
}

// Logarithm
template <floating_arithmetic T>
auto log(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::log, underlying(x));
}

// Exponentiation
template <floating_arithmetic T>
auto exp(const T &x)
{
	using result = decltype(underlying(x));
	return platform_intrinsic_from_args <result> (thunder::exp, underlying(x));
}

// Power
template <native T, typename U>
requires std::is_convertible_v <U, native_t <T>>
native_t <T> pow(const native_t <T> &x, const U &exp)
{
	auto pexp = native_t <T> (exp);
	return platform_intrinsic_from_args <native_t <T>> (thunder::pow, x, pexp);
}

template <native T, size_t N>
vec <T, N> pow(const vec <T, N> &v, const vec <T, N> &exp)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::pow, v, exp);
}

// Dot product
template <vector_arithmetic A, vector_arithmetic B>
// TODO: same_vector_as <....> and so on
requires std::same_as <typename A::arithmetic_type, typename B::arithmetic_type>
auto dot(const A &v, const B &w)
{
	using T = typename A::arithmetic_type::native_type;
	return platform_intrinsic_from_args <native_t <T>> (thunder::dot, v, w);
}

// Cross product
template <native T>
native_t <T> cross(const vec <T, 2> &v, const vec <T, 2> &w)
{
	return v.x * w.y - v.y * w.x;
}

template <native T>
vec <T, 3> cross(const vec <T, 3> &v, const vec <T, 3> &w)
{
	return platform_intrinsic_from_args <vec <T, 3>> (thunder::cross, v, w);
}

// Vector length
template <native T, size_t N>
auto length(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::length, v);
}

// Normalization
template <native T, size_t N>
vec <T, N> normalize(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::normalize, v);
}

// Mix
template <floating_arithmetic T, floating_arithmetic U, floating_arithmetic V>
auto mix(const T &x, const U &y, const V &a)
{
	return platform_intrinsic_from_args <arithmetic_base <T>>
		(thunder::mix, underlying(x), underlying(y), underlying(a));
}

template <floating_native T, size_t N, floating_arithmetic U>
auto mix(const vec <T, N> &x, const vec <T, N> &y, const U &a)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::mix, x, y, underlying(a));
}

// Smoothstep
template <floating_arithmetic T, floating_arithmetic U, floating_arithmetic V>
requires equivalent <T, U>
auto smoothstep(const T &A, const U &B, const V &eta)
{
	using result = decltype(underlying(A));
	return platform_intrinsic_from_args <result> (thunder::smoothstep,
		underlying(A),
		underlying(B),
		underlying(eta));
}

// Discard
inline void discard()
{
	platform_intrinsic_keyword(thunder::discard);
}

// Special control-flow related global states
struct local_size {
	local_size(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) {
		using T = native_t <uint32_t>;

		ire::void_platform_intrinsic_from_args(thunder::layout_local_size, T(x), T(y), T(z));
	}
};

} // namespace jvl::ire
