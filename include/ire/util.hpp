#pragma once

#include "../thunder/atom.hpp"
#include "../thunder/enumerations.hpp"
#include "native.hpp"
#include "tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

template <builtin T, size_t N>
thunder::index_t list_from_array(const std::array <T, N> &args)
{
	auto &em = Emitter::active;

	thunder::List list;

	thunder::index_t next = -1;
	for (int32_t i = N - 1; i >= 0; i--) {
		list.item = args[i].synthesize().id;
		list.next = next;

		next = em.emit(list);
	}

	return next;
}

template <builtin T>
thunder::index_t list_from_vector(const std::vector <T> &args)
{
	auto &em = Emitter::active;

	thunder::List list;

	thunder::index_t next = -1;
	for (int32_t i = args.size() - 1; i >= 0; i--) {
		list.item = args[i].synthesize().id;
		list.next = next;

		next = em.emit(list);
	}

	return next;
}

template <native T, typename ... Args>
thunder::index_t list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::List l;
	l.item = native_t <T> (t).synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <builtin T, typename ... Args>
thunder::index_t list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	thunder::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(args))
		l.next = list_from_args(args...);

	return em.emit(l);
}

template <aggregate T, typename ... Args>
thunder::index_t list_from_args(const T &t, const Args &... args)
{
	auto &em = Emitter::active;

	// TODO: are nested structs handled?
	auto layout = t.layout();

	thunder::Construct ctor;
	ctor.type = type_field_from_args(layout).id;
	ctor.args = layout.list().id;

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
	auto &em = Emitter::active;

	thunder::Operation operation;
	operation.a = a.synthesize().id;
	operation.b = b.synthesize().id;
	operation.code = type;

	cache_index_t cit;
	cit = em.emit(operation);

	return cit;
}

template <builtin R, builtin ... Args>
R platform_intrinsic_from_args(thunder::IntrinsicOperation code, const Args &... args)
{
	auto &em = Emitter::active;
	thunder::index_t operands = list_from_args(args...);
	thunder::index_t intrinsic = em.emit_intrinsic(operands, code);
	return cache_index_t::from(intrinsic);
}

template <builtin ... Args>
void void_platform_intrinsic_from_args(thunder::IntrinsicOperation code, const Args &... args)
{
	auto &em = Emitter::active;
	thunder::index_t operands = list_from_args(args...);
	em.emit_intrinsic(operands, code);
}

inline void platform_intrinsic_keyword(thunder::IntrinsicOperation opn)
{
	auto &em = Emitter::active;

	thunder::Intrinsic intr;
	intr.opn = opn;

	em.emit(intr);
}

template <typename ... Args>
cache_index_t default_construct_index(const const_uniform_layout_t <Args...> &layout, int index)
{
	auto &em = Emitter::active;

	thunder::Construct ctor;
	ctor.type = type_field_index_from_args(index, layout).id;
	// args is (nil) to properly engage the default constructor

	cache_index_t cit;
	cit = em.emit(ctor);
	return cit;
}

template <typename ... Args>
requires (sizeof...(Args) > 0)
cache_index_t const_uniform_layout_t <Args...> ::list() const
{
	auto &em = Emitter::active;

	thunder::List end;

	cache_index_t next = cache_index_t::null();
	for (int i = fields.size() - 1; i >= 0; i--) {
		layout_const_field f = fields[i];

		end.next = next.id;
		if (!f.aggregate) {
			const tagged *t = reinterpret_cast <const tagged *> (f.ptr);

			if (t->ref.id == -1)
				end.item = default_construct_index(*this, i).id;
			else
				end.item = t->ref.id;

		} else {
			MODULE(const-uniform-layout-list);

			JVL_ABORT("nested aggregates are unsupported");
		}

		next = em.emit(end);
	}

	return next;
}

} // namespace jvl::ire
