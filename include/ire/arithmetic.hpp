#pragma once

#include "aliases.hpp"
#include "tagged.hpp"

namespace jvl::ire {

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
