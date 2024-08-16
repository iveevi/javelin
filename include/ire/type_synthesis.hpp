#pragma once

#include "../atom/atom.hpp"
#include "ire/uniform_layout.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

// Forward declarations
template <typename ... Args>
requires (sizeof...(Args) > 0)
struct __uniform_layout;

template <typename T>
constexpr atom::PrimitiveType synthesize_primitive_type();

// Synthesize types for an entire sequence of arguments
template <typename T, typename ... Args>
cache_index_t type_field_from_args()
{
	auto &em = Emitter::active;

	atom::TypeField tf;

	if constexpr (uniform_compatible <T>) {
		using layout_t = decltype(T().layout());
		tf.down = type_field_from_args(layout_t()).id;
	} else {
		tf.item = synthesize_primitive_type <T> ();
	}

	if constexpr (sizeof...(Args))
		tf.next = type_field_from_args <Args...> ().id;
	else
		tf.next = -1;

	cache_index_t cached;
	cached = em.emit(tf);
	return cached;
}

template <typename ... Args>
cache_index_t type_field_from_args(const __uniform_layout <Args...> &args)
{
	return type_field_from_args <Args...> ();
}

template <typename ... Args>
cache_index_t type_field_from_args(const __const_uniform_layout <Args...> &args)
{
	return type_field_from_args <Args...> ();
}

// Synthesize types for a specific index from a sequence of arguments
template <typename T, typename ... Args>
cache_index_t type_field_index_from_args(int index)
{
	if constexpr (sizeof...(Args)) {
		if (index > 0)
			return type_field_index_from_args <Args...> (index);
	}

	auto &em = Emitter::active;

	atom::TypeField tf;

	if constexpr (uniform_compatible <T>) {
		using layout_t = decltype(T().layout());
		tf.down = type_field_from_args(layout_t()).id;
	} else {
		tf.item = synthesize_primitive_type <T> ();
	}

	cache_index_t cached;
	cached = em.emit(tf);
	return cached;
}

template <typename ... Args>
cache_index_t type_field_index_from_args(int index, const __uniform_layout <Args...> &args)
{
	return type_field_index_from_args <Args...> (index);
}

template <typename ... Args>
cache_index_t type_field_index_from_args(int index, const __const_uniform_layout <Args...> &args)
{
	return type_field_index_from_args <Args...> (index);
}

} // namespace jvl::ire
