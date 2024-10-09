#pragma once

#include <type_traits>

#include "../thunder/tracked_buffer.hpp"
#include "control_flow.hpp"
#include "ire/tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

// Procedure flags
enum ProcedureKind {
	eVertex,
	eFragment,
	eCompute,
	eCallable,
};

// Internal construction of procedures
template <ProcedureKind kind, non_trivial_generic_or_void R, typename ... Args>
struct Procedure : thunder::TrackedBuffer {
	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl)
	requires (sizeof...(Args) > 0) {
		auto &em = Emitter::active;

		using type_t = std::decay_t <decltype(std::get <index> (tpl))>;

		if constexpr (aggregate <type_t>) {
			auto &x = std::get <index> (tpl);

			auto layout = x.layout().remove_const();
			thunder::index_t t = type_field_from_args(layout).id;
			thunder::index_t q = em.emit_qualifier(t, index, thunder::parameter);
			thunder::index_t c = em.emit_construct(q, -1, true);
			layout.ref_with(cache_index_t::from(c));
		} else if constexpr (builtin <type_t>) {
			auto &x = std::get <index> (tpl);

			using T = std::decay_t <decltype(x)>;
			
			thunder::index_t t = type_field_from_args <T> ().id;
			thunder::index_t q = em.emit_qualifier(t, index, thunder::parameter);
			x.ref = em.emit_construct(q, -1, true);
		} else {
			// Otherwise treat it as a parameter for Type I partial specialization
		}

		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	void begin() {
		Emitter::active.push(*this);
	}

	void call(std::tuple <Args...> &args)
	requires (sizeof...(Args) > 0) {
		__fill_parameter_references <0> (args);
	}

	void end() {
		Emitter::active.pop();
	}

	auto &named(const std::string &name_) {
		name = name_;
		return *this;
	}

	R operator()(const Args &... args) {
		auto &em = Emitter::active;

		// If R is uniform compatible, then we need to bind its members...
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
		call.cid = cid;
		call.type = type_field_from_args <R> ().id;
		call.args = list_from_args(args...);

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

template <ProcedureKind kind, non_trivial_generic_or_void R, typename ... Args>
struct signature_pair {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using procedure = Procedure <kind, R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_procedure = Procedure <kind, RR, std::decay_t <Args>...>;
};

template <ProcedureKind kind, non_trivial_generic_or_void R>
struct signature_pair <kind, R> {
	using return_t = R;
	using args_t = std::nullopt_t;

	using procedure = Procedure <kind, R>;

	template <typename RR>
	using manual_procedure = Procedure <kind, RR>;
};

template <ProcedureKind kind, typename R, typename ... Args>
auto function_signature_cast(std::function <R (Args...)>)
{
	return signature_pair <kind, R, Args...> ();
}

template <ProcedureKind kind, acceptable_callable F>
auto function_signature(const F &ftn)
{
	return function_signature_cast <kind> (std::function(ftn));
}

#define hacked(T) *reinterpret_cast <T *> ((void *) nullptr)

template <ProcedureKind kind, acceptable_callable F>
struct signature {
	using pair = decltype(function_signature <kind> (hacked(F)));

	using return_t = pair::return_t;
	using args_t = pair::args_t;

	using procedure = pair::procedure;

	template <typename RR>
	using manual_prodecure = typename pair::template manual_procedure <RR>;
};

// Exception for real functions, which cannot be instantiated
template <ProcedureKind kind, non_trivial_generic R, typename ... Args>
struct signature <kind, R (Args...)> {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using procedure = Procedure <kind, R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_procedure = Procedure <kind, RR, std::decay_t <Args>...>;
};

}

// Interface for constructing callables from functions
template <ProcedureKind kind = eCallable, non_trivial_generic_or_void R = void>
struct procedure {
	std::string name;

	procedure(const std::string &name_)
		: name(name_) {}
};

template <ProcedureKind kind, non_trivial_generic_or_void R, typename ... Args>
struct procedure_with_args : procedure <kind, R> {
	std::tuple <Args...> args;

	procedure_with_args(const std::string &name_,
		      const std::tuple <Args...> &args_)
		: procedure <kind, R> (name_), args(args_) {}
};

template <ProcedureKind kind, non_trivial_generic_or_void R, typename ... Args>
auto operator<<(const procedure <kind, R> &C, const std::tuple <Args...> &args)
{
	return procedure_with_args <kind, R, Args...> (C.name, args);
}

template <ProcedureKind kind, non_trivial_generic_or_void R, typename T>
requires (!detail::acceptable_callable <T>)
auto operator<<(const procedure <kind, R> &C, const T &arg)
{
	return procedure_with_args <kind, R, T> (C.name, std::make_tuple(arg));
}

template <ProcedureKind kind, non_trivial_generic_or_void R, detail::acceptable_callable F, typename ... Args>
requires (!std::same_as <typename detail::signature <kind, F> ::return_t, void>)
auto operator<<(const procedure_with_args <kind, R, Args...> &C, F ftn)
{
	using S = detail::signature <kind, F>;

	if constexpr (sizeof...(Args)) {
		// Make sure the types match
		using required = typename S::args_t;
		using provided = decltype(C.args);
		static_assert(std::same_as <required, provided>);
	}

	typename S::procedure cbl;
	cbl.begin();
		if constexpr (std::same_as <typename S::args_t, std::nullopt_t>) {
			auto values = ftn();
			returns(values);
		} else {
			auto args = typename S::args_t();
			if constexpr (sizeof...(Args))
				args = C.args;

			cbl.call(args);
			auto values = std::apply(ftn, args);
			returns(values);
		}
	cbl.end();

	return cbl.named(C.name);
}

// Void functions are presumbed to contain returns(...) statements already
template <ProcedureKind kind, non_trivial_generic_or_void R, detail::acceptable_callable F, typename ... Args>
requires (std::same_as <typename detail::signature <kind, F> ::return_t, void>)
auto operator<<(const procedure_with_args <kind, R, Args...> &C, F ftn)
{
	using S = detail::signature <kind, F>;
	
	if constexpr (sizeof...(Args)) {
		// Make sure the types match
		using required = typename S::args_t;
		using provided = decltype(C.args);
		static_assert(std::same_as <required, provided>,
			"arguments piped to the callable interface "
			"must match the arguments of the function");
	}

	typename S::template manual_prodecure <R> cbl;
	cbl.begin();
		if constexpr (std::same_as <typename S::args_t, std::nullopt_t>) {
			ftn();
		} else {
			auto args = typename S::args_t();
			if constexpr (sizeof...(Args))
				args = C.args;

			cbl.call(args);
			std::apply(ftn, args);
		}
	cbl.end();

	return cbl.named(C.name);
}

template <ProcedureKind kind, non_trivial_generic_or_void R, typename F>
requires detail::acceptable_callable <F>
auto operator<<(const procedure <kind, R> &C, F ftn)
{
	return procedure_with_args <kind, R> (C.name, {}) << ftn;
}

} // namespace jvl::ire
