#pragma once

#include "../concepts.hpp"
#include "signature.hpp"

namespace jvl::ire {

// Interface for constructing callables from functions
// TODO: refactor to procedurebuilder
template <generic_or_void R = void>
struct procedure {
	std::string name;

	procedure(const std::string &name_)
		: name(name_) {}
};

template <generic_or_void R, typename ... Args>
struct procedure_with_args : procedure <R> {
	std::tuple <Args...> args;

	procedure_with_args(const std::string &name_,
		      const std::tuple <Args...> &args_)
		: procedure <R> (name_), args(args_) {}
};

template <generic_or_void R, typename ... Args>
auto operator<<(const procedure <R> &C, const std::tuple <Args...> &args)
{
	return procedure_with_args <R, Args...> (C.name, args);
}

template <generic_or_void R, typename T>
requires (!detail::acceptable_callable <T>)
auto operator<<(const procedure <R> &C, const T &arg)
{
	return procedure_with_args <R, T> (C.name, std::make_tuple(arg));
}

template <generic_or_void R, detail::acceptable_callable F, typename ... Args>
requires (!std::same_as <typename detail::signature <F> ::return_t, void>)
auto operator<<(const procedure_with_args <R, Args...> &C, F ftn)
{
	using S = detail::signature <F>;

	if constexpr (sizeof...(Args)) {
		// Make sure the types match
		using required = typename S::args_t;
		using provided = decltype(C.args);
		static_assert(std::same_as <required, provided>);
	}

	typename S::procedure proc;
	proc.begin();
	{
		auto args = typename S::args_t();
		if constexpr (sizeof...(Args))
			args = C.args;

		proc.call(args);
		auto values = std::apply(ftn, args);
		$return(values);
	}
	proc.end();

	return proc.named(C.name);
}

// Void functions are presumbed to contain returns(...) statements already
template <generic_or_void R, detail::acceptable_callable F, typename ... Args>
requires (std::same_as <typename detail::signature <F> ::return_t, void>)
auto operator<<(const procedure_with_args <R, Args...> &C, F ftn)
{
	using S = detail::signature <F>;

	if constexpr (sizeof...(Args)) {
		// Make sure the types match
		using required = typename S::args_t;
		using provided = decltype(C.args);
		static_assert(std::same_as <required, provided>,
			"arguments piped to the callable interface "
			"must match the arguments of the function");
	}

	typename S::template manual_prodecure <R> proc;
	proc.begin();
	{
		auto args = typename S::args_t();
		if constexpr (sizeof...(Args))
			args = C.args;

		proc.call(args);
		std::apply(ftn, args);
	}
	proc.end();

	return proc.named(C.name);
}

template <generic_or_void R, typename F>
requires detail::acceptable_callable <F>
auto operator<<(const procedure <R> &C, F ftn)
{
	return procedure_with_args <R> (C.name, {}) << ftn;
}

// Short-hand macro for writing shader functions
#define $callable(R, name)	jvl::ire::Procedure name = jvl::ire::procedure <R> (#name) << []

#define $entrypoint(name)	jvl::ire::Procedure <void> name = jvl::ire::procedure("main") << []
 
} // namespace jvl::ire