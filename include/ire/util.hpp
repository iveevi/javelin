#pragma once

#include "primitive.hpp"

namespace jvl::ire {

template <primitive_type T, typename ... Args>
int synthesize_list(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	atom::List l;
	l.item = primitive_t <T> (t).synthesize().id;

	if constexpr (sizeof...(args))
		l.next = synthesize_list(args...);

	return em.emit(l);
}

template <synthesizable T, typename ... Args>
int synthesize_list(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	atom::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(args))
		l.next = synthesize_list(args...);

	return em.emit(l);
}

} // namespace jvl::ire
