#pragma once

#include <cstdlib>
#include <concepts>

namespace jvl::ire {

// Padded elements (backward)
template <typename T, size_t Padding, size_t Index>
struct alignas(4) pad_tuple_leaf {
	char _padding[Padding];
	T data;

	// Accessors
	operator T &() {
		return data;
	}

	operator const T &() const {
		return data;
	}
};

template <typename T, size_t Index>
struct alignas(4) pad_tuple_leaf <T, 0, Index> {
	T data;

	// Accessors
	operator T &() {
		return data;
	}

	operator const T &() const {
		return data;
	}
};

template <typename T, size_t Padding, size_t Index>
struct padded {
	using concrete = pad_tuple_leaf <T, Padding, Index>;
};

template <typename T, size_t Padding, size_t Index>
bool padded_type_pass(const padded <T, Padding, Index> &);

template <typename T>
concept padded_type = requires(const T &value) {
	{ padded_type_pass(value) } -> std::same_as <bool>;
};

template <size_t I, typename... Ts>
struct tuple_element;

template <size_t I, typename T, typename... Ts>
struct tuple_element <I, T, Ts...> : tuple_element <I - 1, Ts...> {};

template <typename T, typename... Ts>
struct tuple_element <0, T, Ts...> {
	using type = typename T::concrete;
};

template <padded_type ... Ts>
struct pad_tuple : Ts::concrete ... {
	template <size_t I>
	auto &get() {
		if constexpr (I < sizeof...(Ts)) {
			using T = typename tuple_element <I, Ts...> ::type;
			return static_cast <T &> (*this).data;
		} else {
			static int32_t value = 0;
			static_assert(false, "tuple index out of bounds");
			return value;
		}
	}

	template <size_t I>
	const auto &get() const {
		if constexpr (I < sizeof...(Ts)) {
			using T = typename tuple_element <I, Ts...> ::type;
			return static_cast <const T &> (*this).data;
		} else {
			static const int32_t value = 0;
			static_assert(false, "tuple index out of bounds");
			return value;
		}
	}
};

} // namespace jvl::ire