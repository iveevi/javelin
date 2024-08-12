#pragma once

#include <array>

#include "tagged.hpp"

namespace jvl::ire {

template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout {
	std::array <tagged *, sizeof...(Args)> fields;

	template <synthesizable T, synthesizable ... UArgs>
	[[gnu::always_inline]]
	void __init(int index, T &t, UArgs &... uargs) {
		// TODO: how to deal with nested structs?
		fields[index] = &static_cast <tagged &> (t);
		if constexpr (sizeof...(uargs) > 0)
			__init(index + 1, uargs...);
	}

	uniform_layout(Args &... args) {
		__init(0, args...);
	}
};

template <typename T>
struct is_uniform_layout {
	static constexpr bool value = false;
};

template <typename ... Args>
struct is_uniform_layout <uniform_layout <Args...>> {
	static constexpr bool value = true;
};

template <typename T>
concept uniform_compatible = is_uniform_layout <decltype(T().layout())> ::value;

} // namespace jvl::ire
