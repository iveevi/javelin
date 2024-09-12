#pragma once

#include "../vector.hpp"
#include "../util.hpp"
#include "../primitive.hpp"
#include "thunder/enumerations.hpp"
#include <type_traits>

namespace jvl::ire {

template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> max(const primitive_t <T> &a, const U &b)
{
	auto pb = primitive_t <T> (b);
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::max, a, pb);
}

template <native T>
primitive_t <T> min(const primitive_t <T> &a, const primitive_t <T> &b)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::min, a, b);
}

template <native T>
primitive_t <T> sqrt(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::sqrt, x);
}

template <native T, typename Up, thunder::SwizzleCode swz>
primitive_t <T> sqrt(const swizzle_element <T, Up, swz> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::sqrt, x);
}

template <native T, typename U, typename V>
requires (std::is_convertible_v <U, primitive_t <T>>) && (std::is_convertible_v <V, primitive_t <T>>)
primitive_t <T> clamp(const primitive_t <T> &x, const U &min, const V &max)
{
	auto pmin = primitive_t <T> (min);
	auto pmax = primitive_t <T> (max);
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::clamp, x, pmin, pmax);
}

template <native T>
primitive_t <T> sin(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::sin, x);
}

template <native T>
primitive_t <T> cos(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::cos, x);
}

template <native T>
primitive_t <T> tan(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::tan, x);
}

template <native T>
primitive_t <T> asin(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::asin, x);
}

template <native T>
primitive_t <T> acos(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::acos, x);
}

template <native T>
primitive_t <T> atan(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::atan, x);
}

template <native T, typename U>
requires std::is_convertible_v <U, primitive_t <T>>
primitive_t <T> pow(const primitive_t <T> &x, const U &exp)
{
	auto pexp = primitive_t <T> (exp);
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::pow, x, pexp);
}

template <native T, size_t N>
vec <T, N> pow(const vec <T, N> &v, const primitive_t <T> &exp)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::pow, v, exp);
}

template <native T, size_t N>
vec <T, N> clamp(const vec <T, N> &x, const primitive_t <T> &min, const primitive_t <T> &max)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::clamp, x, min, max);
}

template <typename T, size_t N>
primitive_t <T> dot(const vec <T, N> &v, const vec <T, N> &w)
{
	return platform_intrinsic_from_args <primitive_t <T>> (thunder::dot, v, w);
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
