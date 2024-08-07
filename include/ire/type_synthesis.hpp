#pragma once

#include "op.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

// Forward declarations
template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout;

template <typename T>
constexpr op::PrimitiveType primitive_type();

template <typename T>
int type_field()
{
	auto &em = Emitter::active;

	// TODO: for all types
	op::TypeField type;
	type.item = primitive_type <T> ();
	type.next = -1;

	return em.emit(type);
}

template <typename T, typename ... Args>
cache_index_t synthesize_type_fields()
{
	static thread_local cache_index_t cached = cache_index_t::null();
	if (cached.id != -1)
		return cached;

	auto &em = Emitter::active;

	op::TypeField tf;
	tf.item = primitive_type <T> ();
	if constexpr (sizeof...(Args))
		tf.next = synthesize_type_fields <Args...> ().id;
	else
		tf.next = -1;

	return (cached = em.emit(tf));
}

template <typename ... Args>
cache_index_t synthesize_type_fields(const uniform_layout <Args...> &args)
{
	return synthesize_type_fields <Args...> ();
}

} // namespace jvl::ire
