#pragma once

#include "imports.hpp"
#include "intrinsics.hpp"

namespace jvl::tsg {

namespace detail {

// Recursive initialization of lifted arguments
template <typename T, typename ... Args>
struct shader_args_initializer {
	static_assert(false);
};

// Generic handler, does nothing
template <typename R, typename ... Refs, typename T, typename ... Args>
struct shader_args_initializer <std::tuple <R, Refs...>, T, Args...> {
	static_assert(sizeof...(Refs) == sizeof...(Args));

	static void run(size_t binding, T &current, Args &...args) {
		if constexpr (sizeof...(args))
			return shader_args_initializer <std::tuple <Refs...>, Args...> ::run(binding, args...);
	}
};

// Transfer correct bindings to layout inputs (and increment the counter)
template <typename R, typename ... Refs, generic T, typename ... Args>
struct shader_args_initializer <std::tuple <R, Refs...>, ire::layout_in <T>, Args...> {
	static_assert(sizeof...(Refs) == sizeof...(Args));

	static void run(size_t binding, ire::layout_in <T> &current, Args &... args) {
		current = ire::layout_in <T> (binding);
		if constexpr (sizeof...(args))
			return shader_args_initializer <std::tuple <Refs...>, Args...> ::run(binding + 1, args...);
	}
};

// Apply the correct offset to push constants
template <generic T, size_t Offset, typename ... Refs, typename ... Args>
struct shader_args_initializer <std::tuple <PushConstant <T, Offset>, Refs...>, ire::push_constant <T>, Args...> {
	static_assert(sizeof...(Refs) == sizeof...(Args));

	static void run(size_t binding, ire::push_constant <T> &current, Args &... args) {
		current = ire::push_constant <T> (Offset);
		if constexpr (sizeof...(args))
			return shader_args_initializer <std::tuple <Refs...>, Args...> ::run(binding, args...);
	}
};

template <typename Ref, typename ... Args>
void shader_args_initializer_do(Args &... args)
{
	return shader_args_initializer <Ref, Args...> ::run(0, args...);
}

}

template <typename Ref, typename ... Args>
void shader_args_initializer(const Ref &refs, std::tuple <Args...> &args)
{
	return std::apply(detail::shader_args_initializer_do <Ref, Args...>, args);
}

} // namespace jvl::tsg