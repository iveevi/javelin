#pragma once

#include "aliases.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// TODO: quaternion class
template <primitive_type T, size_t N>
vec <T, N> operator+(const T &k, const vec <T, N> &v)
{
	return operation_from_args <vec <T, N>> (atom::Operation::addition, k, v);
}

template <primitive_type T, size_t N>
vec <T, N> operator*(const T &k, const vec <T, N> &v)
{
	return operation_from_args <vec <T, N>> (atom::Operation::multiplication, k, v);
}

template <primitive_type T, size_t N>
vec <T, N> operator*(const primitive_t <T> &k, const vec <T, N> &v)
{
	return operation_from_args <vec <T, N>> (atom::Operation::multiplication, k, v);
}

template <typename T, size_t M, size_t N>
vec <T, M> operator*(const mat <T, M, N> &m, const vec <T, N> &v)
{
	return operation_from_args <vec <T, M>> (atom::Operation::multiplication, m, v);
}

} // namespace jvl::ire
