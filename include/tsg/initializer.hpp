#pragma once

#include "imports.hpp"

namespace jvl::tsg {

namespace detail {

// Recursive initialization of lifted arguments
template <typename T, typename ... Args>
struct shader_args_initializer {
	static void run(size_t binding, T &current, Args &...args) {
		if constexpr (sizeof...(args))
			return shader_args_initializer <Args...> ::run(binding, args...);
	}
};

template <typename T, typename ... Args>
struct shader_args_initializer <ire::layout_in <T>, Args...> {
	static void run(size_t binding, ire::layout_in <T> &current, Args &... args) {
		current = ire::layout_in <T> (binding);
		if constexpr (sizeof...(args))
			return shader_args_initializer <Args...> ::run(binding + 1, args...);
	}
};

template <typename ... Args>
void shader_args_initializer_do(Args &... args)
{
	return shader_args_initializer <Args...> ::run(0, args...);
}

}

template <typename ... Args>
void shader_args_initializer(std::tuple <Args...> &args)
{
	return std::apply(detail::shader_args_initializer_do <Args...>, args);
}

} // namespace jvl::tsg