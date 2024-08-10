#pragma once

#include "atom.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

// Forward declarations
template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout;

template <typename T>
constexpr atom::PrimitiveType synthesize_primitive_type();

template <typename T, typename ... Args>
cache_index_t type_field_from_args()
{
	static thread_local cache_index_t cached = cache_index_t::null();
	if (cached.id != -1)
		return cached;

	auto &em = Emitter::active;

	atom::TypeField tf;
	tf.item = synthesize_primitive_type <T> ();
	if constexpr (sizeof...(Args))
		tf.next = type_field_from_args <Args...> ().id;
	else
		tf.next = -1;

	cached = em.emit(tf);
	return em.persist_cache_index(cached);
}

template <typename ... Args>
cache_index_t type_field_from_args(const uniform_layout <Args...> &args)
{
	return type_field_from_args <Args...> ();
}

} // namespace jvl::ire
