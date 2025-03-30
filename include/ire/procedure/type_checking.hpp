#pragma once

#include <concepts>

#include "../concepts.hpp"

namespace jvl::ire {

//////////////////////////////////////////
// Type checking partial specialization //
//////////////////////////////////////////

template <typename Provided, typename Required>
struct is_type_slice : std::false_type {
	static constexpr bool generics = false;

	using remainder = void;
};

// Mismatching frontal types
template <typename T, typename U, typename ... As, typename ... Bs>
struct is_type_slice <std::tuple <T, As...>, std::tuple <U, Bs...>> : std::false_type {
	static constexpr bool generics = false;

	using remainder = void;
};

// Matching frontal types (first type only)
template <typename T, typename U, typename ... As, typename ... Bs>
requires std::same_as <std::decay_t <T>, std::decay_t <U>>
struct is_type_slice <std::tuple <T, As...>, std::tuple <U, Bs...>> {
	using as_t = std::tuple <As...>;
	using bs_t = std::tuple <Bs...>;

	using slice_eval_t = is_type_slice <as_t, bs_t>;

	static constexpr bool value = slice_eval_t::value;
	static constexpr bool generics = slice_eval_t::generics;

	using remainder = slice_eval_t::remainder;
};

// Completed match
template <typename ... Bs>
struct is_type_slice <std::tuple <>, std::tuple <Bs...>> : std::true_type {
	static constexpr bool generics = (generic <Bs> && ...);

	using remainder = std::tuple <Bs...>;
};

} // namespace jvl::ire