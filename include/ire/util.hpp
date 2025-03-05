#pragma once

#include "../common/logging.hpp"

#include "../thunder/atom.hpp"
#include "../thunder/enumerations.hpp"

#include "native.hpp"
#include "tagged.hpp"
#include "layout.hpp"

namespace jvl::ire {

template <builtin T, size_t N>
thunder::Index list_from_array(const std::array <T, N> &args)
{
	auto &em = Emitter::active;

	thunder::List list;

	thunder::Index next = -1;
	for (int32_t i = N - 1; i >= 0; i--) {
		list.item = args[i].synthesize().id;
		list.next = next;

		next = em.emit(list);
	}

	return next;
}

template <builtin T>
thunder::Index list_from_vector(const std::vector <T> &args)
{
	auto &em = Emitter::active;

	thunder::List list;

	thunder::Index next = -1;
	for (int32_t i = args.size() - 1; i >= 0; i--) {
		list.item = args[i].synthesize().id;
		list.next = next;

		next = em.emit(list);
	}

	return next;
}

template <native T, typename ... Args>
thunder::Index list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::List l;
	l.item = native_t <T> (t).synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <builtin T, typename ... Args>
thunder::Index list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <aggregate T, typename ... Args>
thunder::Index list_from_args(T &t, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::Construct ctor;

	auto layout = t.layout();
	ctor.type = layout.generate_type().concrete();
	ctor.args = layout.reconstruct();

	thunder::List l;
	l.item = em.emit(ctor);

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <builtin R, builtin A>
R operation_from_args(thunder::OperationCode type, const A &a)
{
	auto &em = Emitter::active;

	thunder::Operation operation;
	operation.a = a.synthesize().id;
	operation.code = type;

	cache_index_t cit;
	cit = em.emit(operation);

	return cit;
}

template <builtin R, builtin A, builtin B>
R operation_from_args(thunder::OperationCode type, const A &a, const B &b)
{
	MODULE(operation_from_args);

	auto &em = Emitter::active;

	size_t size = em.scopes.top().get().pointer;

	thunder::Index aid = a.synthesize().id;

	JVL_ASSERT(aid >= 0 && (size_t) aid <= size,
		"invalid index ({}) synthesized for operation (type: {})",
		aid, thunder::tbl_operation_code[type]);

	thunder::Index bid = b.synthesize().id;
	thunder::Index rit = em.emit_operation(aid, bid, type);

	return cache_index_t::from(rit);
}

template <builtin R, builtin ... Args>
R platform_intrinsic_from_args(thunder::IntrinsicOperation code, const Args &... args)
{
	auto &em = Emitter::active;
	thunder::Index operands = list_from_args(args...);
	thunder::Index intrinsic = em.emit_intrinsic(operands, code);
	return cache_index_t::from(intrinsic);
}

template <builtin ... Args>
void void_platform_intrinsic_from_args(thunder::IntrinsicOperation code, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::Index operands = -1;
	if constexpr (sizeof...(Args))
		operands = list_from_args(args...);
		
	em.emit_intrinsic(operands, code);
}

inline void platform_intrinsic_keyword(thunder::IntrinsicOperation opn)
{
	auto &em = Emitter::active;

	thunder::Intrinsic intr;
	intr.opn = opn;

	em.emit(intr);
}

} // namespace jvl::ire
