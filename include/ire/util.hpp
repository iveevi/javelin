#pragma once

#include "gltype.hpp"

namespace jvl::ire {

template <gltype_complatible T, typename ... Args>
int synthesize_list(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	op::List l;
	l.item = gltype <T> (t).synthesize().id;

	if constexpr (sizeof...(args))
		l.next = synthesize_list(args...);

	return em.emit(l);
}

template <synthesizable T, typename ... Args>
int synthesize_list(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	op::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(args))
		l.next = synthesize_list(args...);

	return em.emit(l);
}

} // namespace jvl::ire
