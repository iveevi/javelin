#pragma once

#include "../vector.hpp"
#include "../util.hpp"
#include "../primitive.hpp"

namespace jvl::ire {

template <primitive_type T>
primitive_t <T> acos(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> ("acos", x);
}

template <primitive_type T>
primitive_t <T> cos(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> ("cos", x);
}

template <primitive_type T>
primitive_t <T> tan(const primitive_t <T> &x)
{
	return platform_intrinsic_from_args <primitive_t <T>> ("tan", x);
}

template <primitive_type T>
primitive_t <T> pow(const primitive_t <T> &x, const primitive_t <T> &exp)
{
	return platform_intrinsic_from_args <primitive_t <T>> ("pow", x, exp);
}

template <primitive_type T, size_t N>
vec <T, N> pow(const vec <T, N> &v, const primitive_t <T> &exp)
{
	return platform_intrinsic_from_args <vec <T, N>> ("pow", v, exp);
}

template <primitive_type T, size_t N>
vec <T, N> clamp(const vec <T, N> &x, const primitive_t <T> &min, const primitive_t <T> &max)
{
	return platform_intrinsic_from_args <vec <T, N>> ("clamp", x, min, max);
}

template <typename T, size_t N>
primitive_t <T> dot(const vec <T, N> &v, const vec <T, N> &w)
{
	return platform_intrinsic_from_args <primitive_t <T>> ("dot", v, w);
}

template <primitive_type T>
vec <T, 3> cross(const vec <T, 3> &v, const vec <T, 3> &w)
{
	return platform_intrinsic_from_args <vec <T, 3>> ("cross", v, w);
}

template <primitive_type T, size_t N>
vec <T, N> normalize(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> ("normalize", v);
}

inline void discard()
{
	platform_intrinsic_keyword("discard");
}

} // namespace jvl::ire
