#pragma once

#include <functional>

namespace jvl::tsg {

// Resolving the function meta information
#define hacked(T) *reinterpret_cast <T *> ((void *) nullptr)

template <typename F>
auto std_function(const F &ftn)
{
	return std::function(ftn);
}

template <typename F>
struct function_type_breakdown {
	using base = decltype(std_function(hacked(F)));
};

template <typename F>
auto function_breakdown(const F &ftn)
{
	return function_type_breakdown <F> ();
}

template <typename F>
struct signature {};

template <typename R, typename ... Args>
struct signature <std::function <R (Args...)>> {
	using returns = R;
	using arguments = std::tuple <std::decay_t <Args>...>;
};

} // namespace jvl::tsg
