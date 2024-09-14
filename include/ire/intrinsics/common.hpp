#pragma once

#include "../vector.hpp"
#include "../util.hpp"
#include "../primitive.hpp"
#include "thunder/enumerations.hpp"
#include <type_traits>

namespace jvl::ire {

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

template <native T>
native_t <T> atan(const native_t <T> &x)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::atan, x);
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
vec <T, N> normalize(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::normalize, v);
}

inline void discard()
{
	platform_intrinsic_keyword(thunder::discard);
}

} // namespace jvl::ire
