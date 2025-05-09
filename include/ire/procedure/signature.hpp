#pragma once

#include <functional>
#include <tuple>

#include "../../common/meta.hpp"
#include "../concepts.hpp"

namespace jvl::ire {

// Forward declarations
template <generic_or_void R, typename ... Args>
struct Procedure;
	
namespace detail {

template <typename F>
concept acceptable_callable = std::is_function_v <F> || requires(const F &ftn) {
	{ std::function(ftn) };
};

template <generic_or_void R, typename ... Args>
struct signature_pair {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;
	using packet = meta::type_packet <Args...>;

	using procedure = Procedure <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_procedure = Procedure <RR, std::decay_t <Args>...>;
};

template <typename R, typename ... Args>
auto function_signature_cast(std::function <R (Args...)>)
{
	return signature_pair <R, Args...> ();
}

template <acceptable_callable F>
auto function_signature(const F &ftn)
{
	return function_signature_cast(std::function(ftn));
}

#define hacked(T) *reinterpret_cast <T *> ((void *) nullptr)

template <acceptable_callable F>
struct signature {
	using pair = decltype(function_signature(hacked(F)));

	using return_t = pair::return_t;
	using args_t = pair::args_t;
	using packet = pair::packet;

	using procedure = pair::procedure;

	template <typename RR>
	using manual_prodecure = typename pair::template manual_procedure <RR>;
};

// Exception for real functions, which cannot be instantiated
template <generic R, typename ... Args>
struct signature <R (Args...)> {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;
	using packet = meta::type_packet <Args...>;

	using procedure = Procedure <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_procedure = Procedure <RR, std::decay_t <Args>...>;
};

} // namespace detail

} // namespace jvl::ire