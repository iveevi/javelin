#pragma once

#include <cstdlib>

namespace jvl::ire {

// Indexing type
template <size_t>
struct aggregate_index {};

// Storage type for aggregates
template <size_t N, typename T, typename ... Args>
struct aggregate_base : aggregate_base <N + 1, Args...> {
	T v = T();

	template <size_t I>
	auto &get() {
		if constexpr (I == N)
			return v;
		else
			return aggregate_base <N + 1, Args...> ::template get <I> ();
	}
	
	template <size_t I>
	const auto &get() const {
		if constexpr (I == N)
			return v;
		else
			return aggregate_base <N + 1, Args...> ::template get <I> ();
	}
};

template <size_t N, typename T>
struct aggregate_base <N, T> {
	T v = T();

	template <size_t I>
	auto &get() {
		static_assert(I == N, "aggregate index is out of bounds");
		return v;
	}

	template <size_t I>
	const auto &get() const {
		static_assert(I == N, "aggregate index is out of bounds");
		return v;
	}
};

// Reversing variadic parameter lists
template <typename ... Args>
struct variadic_list {
	template <typename T>
	using append = variadic_list <Args..., T>;

	using aggregate = aggregate_base <0, Args...>;
};

template <>
struct variadic_list <> {
	template <typename T>
	using append = variadic_list <T>;
};

template <typename ... Args>
struct aggregrate_generator {};

template <>
struct aggregrate_generator <> {
	using type = variadic_list <>;
};

template <typename T, typename ... Args>
struct aggregrate_generator <T, Args...> {
	using type = aggregrate_generator <Args...> ::type::template append <T>;
};

template <typename ... Args>
using corrected_aggregate = aggregrate_generator <Args...> ::type::aggregate;

// User-level construct for intefacing the aggregates
template <typename ... Args>
struct aggregate_storage : corrected_aggregate <Args...> {
	template <size_t I>
	auto &get() {
		constexpr size_t N = sizeof...(Args);
		return corrected_aggregate <Args...> ::template get <N - I - 1> ();
	}
	
	template <size_t I>
	const auto &get() const {
		constexpr size_t N = sizeof...(Args);
		return corrected_aggregate <Args...> ::template get <N - I - 1> ();
	}
	
	template <size_t I>
	auto &get(aggregate_index <I>) {
		return get <I> ();
	}
	
	template <size_t I>
	const auto &get(aggregate_index <I>) const {
		return get <I> ();
	}
	
	template <size_t I>
	auto &operator[](aggregate_index <I>) {
		return get <I> ();
	}
	
	template <size_t I>
	const auto &operator[](aggregate_index <I>) const {
		return get <I> ();
	}
};

// For completeness, add the empty version
template <>
struct aggregate_storage <> {};

} // namespace jvl::ire
