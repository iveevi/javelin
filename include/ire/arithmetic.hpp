#pragma once

#include "aliases.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// TODO: quaternion class?
template <primitive_type T, size_t N>
vec <T, N> operator+(const T &k, const vec <T, N> &v)
{
	auto &em = Emitter::active;

	atom::Operation op;
	op.type = atom::Operation::addition;
	op.args = synthesize_list(k, v);

	cache_index_t cit;
	cit = em.emit(op);

	return cit;
}

template <primitive_type T, size_t N>
vec <T, N> operator*(const T &k, const vec <T, N> &v)
{
	auto &em = Emitter::active;

	atom::Operation op;
	op.type = atom::Operation::multiplication;
	op.args = synthesize_list(k, v);

	cache_index_t cit;
	cit = em.emit(op);

	return cit;
}

template <primitive_type T, size_t N>
vec <T, N> operator*(const primitive_t <T> &k, const vec <T, N> &v)
{
	auto &em = Emitter::active;

	atom::Operation op;
	op.type = atom::Operation::multiplication;
	op.args = synthesize_list(k, v);

	cache_index_t cit;
	cit = em.emit(op);

	return cit;
}

inline vec4 operator*(const mat4 &m, const vec4 &v)
{
	auto &em = Emitter::active;

	atom::Operation op;
	op.type = atom::Operation::multiplication;
	op.args = synthesize_list(m, v);

	cache_index_t cit;
	cit = em.emit(op);

	return cit;
}

} // namespace jvl::ire
