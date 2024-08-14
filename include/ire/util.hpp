#pragma once

#include "atom/atom.hpp"
#include "ire/type_synthesis.hpp"
#include "primitive.hpp"

namespace jvl::ire {

template <primitive_type T, typename ... Args>
int list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	atom::List l;
	l.item = primitive_t <T> (t).synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <synthesizable T, typename ... Args>
int list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	atom::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <typename R, typename ... Args>
R operation_from_args(decltype(atom::Operation::type) type, const Args &... args)
{
	auto &em = Emitter::active;

	atom::Operation intr;
	intr.type = type;
	intr.args = list_from_args(args...);
	intr.ret = type_field_from_args <Args...> ().id;

	cache_index_t cit;
	cit = em.emit(intr);

	return cit;
}

template <synthesizable R, synthesizable ... Args>
R platform_intrinsic_from_args(const char *name, const Args &... args)
{
	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;
	intr.args = list_from_args(args...);
	intr.ret = type_field_from_args <Args...> ().id;

	cache_index_t cit;
	cit = em.emit_main(intr);

	return cit;
}

template <synthesizable ... Args>
void void_platform_intrinsic_from_args(const char *name, const Args &... args)
{
	auto &em = Emitter::active;

	atom::TypeField tf;
	tf.item = atom::PrimitiveType::none;

	atom::Intrinsic intr;
	intr.name = name;
	intr.ret = em.emit(tf);

	if constexpr (sizeof...(Args))
		intr.args = list_from_args(args...);

	em.emit_main(intr);
}

inline void platform_intrinsic_keyword(const char *name)
{
	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;

	em.emit_main(intr);
}

} // namespace jvl::ire
