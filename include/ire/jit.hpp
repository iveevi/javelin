#pragma once

#include "../thunder/linkage_unit.hpp"
#include "procedure.hpp"
#include "solidify.hpp"

namespace jvl::ire {

// template <typename T>
// constexpr void jit_check_return()
// {
// 	static_assert(solidifiable <T>, "function to be JIT-ed must have a return type that is solidifiable");
// }

// template <typename T, typename ... Args>
// constexpr void jit_check_arguments()
// {
// 	static_assert(solidifiable <T>, "function to be JIT-ed must have arguments which are solidifiable");
// 	if constexpr (solidifiable <T> && sizeof...(Args))
// 		return jit_check_arguments <Args...> ();
// }

template <typename R, typename ... Args>
auto jit(const Procedure <eCallable, R, Args...> &callable)
{
	// jit_check_return <R> ();
	// jit_check_arguments <Args...> ();

	using function_t = solid_t <R> (*)(solid_t <Args> ...);

	thunder::LinkageUnit unit;
	unit.add(callable);
	auto ftn = unit.jit();

	return function_t(ftn);
}

} // namespace jvl::ire
