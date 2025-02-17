#pragma once

#include <type_traits>

#include "../thunder/optimization.hpp"
#include "../thunder/tracked_buffer.hpp"
#include "control_flow.hpp"
#include "parameters.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// Generating type codes and filling references for parameters
template <typename T>
struct parameter_injection : std::false_type {};

template <builtin T>
struct parameter_injection <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void inject(T &x, thunder::Index i) {
		x.ref = i;
	}
};

template <aggregate T>
struct parameter_injection <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void inject(T &x, thunder::Index i) {
		x.layout().link(i);
	}
};

template <generic T>
struct parameter_injection <in <T>> : std::true_type {
	using U = promoted <T>;
	using P = parameter_injection <U>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_in);
	}

	static void inject(in <T> &x, thunder::Index i) {
		U &y = static_cast <U &> (x);
		P::inject(y, i);
	}
};

template <generic T>
struct parameter_injection <out <T>> : std::true_type {
	using U = promoted <T>;
	using P = parameter_injection <U>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_out);
	}

	static void inject(out <T> &x, thunder::Index i) {
		U &y = static_cast <U &> (x);
		P::inject(y, i);
	}
};

template <generic T>
struct parameter_injection <inout <T>> : std::true_type {
	using U = promoted <T>;
	using P = parameter_injection <U>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_inout);
	}

	static void inject(inout <T> &x, thunder::Index i) {
		U &y = static_cast <U &> (x);
		P::inject(y, i);
	}
};

// Internal construction of procedures
template <void_or_non_native_generic R, typename ... Args>
struct Procedure : thunder::TrackedBuffer {
	// TODO: put outside this defn
	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl)
	requires (sizeof...(Args) > 0) {
		auto &em = Emitter::active;

		using T = std::decay_t <decltype(std::get <index> (tpl))>;
		using P = parameter_injection <T>;

		auto &x = std::get <index> (tpl);

		// Parameters in the target language
		if constexpr (P::value) {
			thunder::Index t = P::emit();
			thunder::Index q = em.emit_qualifier(t, index, thunder::parameter);
			thunder::Index c = em.emit_construct(q, -1, thunder::transient);
			P::inject(x, c);
		}

		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	void begin() {
		Emitter::active.push(*this);
	}

	void call(std::tuple <Args...> &args) {
		if constexpr (sizeof...(Args))
			__fill_parameter_references <0> (args);
	}

	void end() {
		Emitter::active.pop();
	}

	auto &named(const std::string &name_) {
		name = name_;
		return *this;
	}

	R operator()(Args ... args) {
		auto &em = Emitter::active;

		// If R is an aggregate, we need to bind its members
		if constexpr (aggregate <R>) {
			R instance;

			auto layout = instance.layout().remove_const();

			thunder::Call call;
			call.cid = cid;
			call.type = type_field_from_args(layout).id;
			call.args = list_from_args(args...);

			cache_index_t cit;
			cit = em.emit(call);

			layout.ref_with(cit);
			return instance;
		}

		// For primitives
		thunder::Call call;

		auto r = R();

		call.cid = cid;
		call.args = list_from_args(args...);
		call.type = type_info_generator <R> (r)
			.synthesize()
			.concrete();

		cache_index_t cit;
		cit = em.emit(call);
		
		return R(cit);
	}
};

// Metaprogramming to generate the correct callable_t for native function types
namespace detail {

// TODO: find another base than std function
template <typename F>
concept acceptable_callable = std::is_function_v <F> || requires(const F &ftn) {
	{ std::function(ftn) };
};

template <void_or_non_native_generic R, typename ... Args>
struct signature_pair {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

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

	using procedure = pair::procedure;

	template <typename RR>
	using manual_prodecure = typename pair::template manual_procedure <RR>;
};

// Exception for real functions, which cannot be instantiated
template <non_native_generic R, typename ... Args>
struct signature <R (Args...)> {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using procedure = Procedure <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_procedure = Procedure <RR, std::decay_t <Args>...>;
};

}

// Interface for constructing callables from functions
// TODO: optimization options
template <void_or_non_native_generic R = void>
struct procedure {
	std::string name;

	procedure(const std::string &name_)
		: name(name_) {}
};

template <void_or_non_native_generic R, typename ... Args>
struct procedure_with_args : procedure <R> {
	std::tuple <Args...> args;

	procedure_with_args(const std::string &name_,
		      const std::tuple <Args...> &args_)
		: procedure <R> (name_), args(args_) {}
};

template <void_or_non_native_generic R, typename ... Args>
auto operator<<(const procedure <R> &C, const std::tuple <Args...> &args)
{
	return procedure_with_args <R, Args...> (C.name, args);
}

template <void_or_non_native_generic R, typename T>
requires (!detail::acceptable_callable <T>)
auto operator<<(const procedure <R> &C, const T &arg)
{
	return procedure_with_args <R, T> (C.name, std::make_tuple(arg));
}

template <void_or_non_native_generic R, detail::acceptable_callable F, typename ... Args>
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
		returns(values);
	}
	proc.end();

	// thunder::opt_transform(proc);

	return proc.named(C.name);
}

// Void functions are presumbed to contain returns(...) statements already
template <void_or_non_native_generic R, detail::acceptable_callable F, typename ... Args>
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

	// thunder::opt_transform(proc);

	return proc.named(C.name);
}

template <void_or_non_native_generic R, typename F>
requires detail::acceptable_callable <F>
auto operator<<(const procedure <R> &C, F ftn)
{
	return procedure_with_args <R> (C.name, {}) << ftn;
}

} // namespace jvl::ire
