#pragma once

#include "callable.hpp"
#include "solidify.hpp"

namespace jvl::ire {

template <typename T>
constexpr void jit_check_return()
{
	static_assert(solidifiable <T>, "function to be JIT-ed must have a return type that is solidifiable");
}

template <typename T, typename ... Args>
constexpr void jit_check_arguments()
{
	static_assert(solidifiable <T>, "function to be JIT-ed must have a arguments that are solidifiable");
	if constexpr (solidifiable <T> && sizeof...(Args))
		return jit_check_arguments <Args...> ();
}

template <typename R, typename ... Args>
auto jit(const callable_t <R, Args...> &callable)
{
	jit_check_return <R> ();
	jit_check_arguments <Args...> ();

	using function_t = solid_t <R> (*)(solid_t <Args> ...);
	auto kernel = callable.export_to_kernel();
	auto linkage = kernel.linkage().resolve();
	auto jr = linkage.generate_jit_gcc();
	return function_t(jr.result);
}

} // namespace jvl::ire
