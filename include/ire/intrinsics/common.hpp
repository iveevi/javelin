#pragma once

#include "../../thunder/enumerations.hpp"
#include "../native.hpp"
#include "../util.hpp"
#include "../vector.hpp"

namespace jvl::ire {

template <arithmetic T>
auto abs(const T &a)
{
	using result = decltype(underlying(a));
	return platform_intrinsic_from_args <result> (thunder::abs, a);
}

template <native T, typename U>
requires std::is_convertible_v <U, native_t <T>>
native_t <T> max(const native_t <T> &a, const U &b)
{
	auto pb = native_t <T> (b);
	return platform_intrinsic_from_args <native_t <T>> (thunder::max, a, pb);
}

template <native T>
native_t <T> min(const native_t <T> &a, const native_t <T> &b)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::min, a, b);
}

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

template <native T, typename U, typename V>
requires (std::is_convertible_v <U, native_t <T>>) && (std::is_convertible_v <V, native_t <T>>)
native_t <T> clamp(const native_t <T> &x, const U &min, const V &max)
{
	auto pmin = native_t <T> (min);
	auto pmax = native_t <T> (max);
	return platform_intrinsic_from_args <native_t <T>> (thunder::clamp, x, pmin, pmax);
}

template <floating_arithmetic T>
auto floor(const T &a)
{
	using result = decltype(underlying(a));
	return platform_intrinsic_from_args <result> (thunder::floor, a);
}

template <floating_arithmetic T>
auto ceil(const T &a)
{
	using result = decltype(underlying(a));
	return platform_intrinsic_from_args <result> (thunder::ceil, a);
}

template <floating_arithmetic T>
auto fract(const T &a)
{
	using result = decltype(underlying(a));
	return platform_intrinsic_from_args <result> (thunder::fract, a);
}

template <native T>
native_t <T> sin(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::sin, x);
}

template <native T>
native_t <T> cos(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::cos, x);
}

template <native T>
native_t <T> tan(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::tan, x);
}

template <native T>
native_t <T> asin(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::asin, x);
}

template <native T>
native_t <T> acos(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::acos, x);
}

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

template <native T>
native_t <T> log(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::log, x);
}

template <native T>
native_t <T> exp(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::exp, x);
}

template <native T, typename U>
requires std::is_convertible_v <U, native_t <T>>
native_t <T> pow(const native_t <T> &x, const U &exp)
{
	auto pexp = native_t <T> (exp);
	return platform_intrinsic_from_args <native_t <T>> (thunder::pow, x, pexp);
}

template <native T, size_t N>
vec <T, N> pow(const vec <T, N> &v, const native_t <T> &exp)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::pow, v, exp);
}

template <native T, size_t N>
vec <T, N> clamp(const vec <T, N> &x, const native_t <T> &min, const native_t <T> &max)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::clamp, x, min, max);
}

template <typename T, size_t N>
native_t <T> dot(const vec <T, N> &v, const vec <T, N> &w)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::dot, v, w);
}

template <native T>
vec <T, 3> cross(const vec <T, 3> &v, const vec <T, 3> &w)
{
	return platform_intrinsic_from_args <vec <T, 3>> (thunder::cross, v, w);
}

template <native T, size_t N>
auto length(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::length, v);
}

template <native T, size_t N>
vec <T, N> normalize(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::normalize, v);
}

template <native T, size_t N, arithmetic U>
requires std::same_as <typename arithmetic_base <U> ::native_type, float>
vec <T, N> mix(const vec <T, N> &x, const vec <T, N> &y, const U &a)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::mix, x, y, underlying(a));
}

inline void discard()
{
	platform_intrinsic_keyword(thunder::discard);
}

} // namespace jvl::ire
