#pragma once

#include "../thunder/atom.hpp"
#include "primitive.hpp"
#include "uniform_layout.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

// Forward declarations
template <typename T>
constexpr thunder::PrimitiveType synthesize_primitive_type();

// Fundamental types for IRE
template <typename T>
concept generic = primitive_type <T> || synthesizable <T> || uniform_compatible <T>;

template <typename T>
concept non_trivial_generic = synthesizable <T> || uniform_compatible <T>;

// Synthesize types for an entire sequence of arguments
template <typename T, typename ... Args>
cache_index_t type_field_from_args()
{
	auto &em = Emitter::active;

	thunder::TypeField tf;

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

	thunder::TypeField tf;

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
