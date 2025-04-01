#pragma once

#include <type_traits>

#include "../../thunder/tracked_buffer.hpp"
#include "../control_flow.hpp"
#include "../tagged.hpp"
#include "injection.hpp"
#include "signature.hpp"

namespace jvl::ire {

namespace detail {

template <size_t index, typename ... Args>
requires (sizeof...(Args) > 0)
void inject_parameters(std::tuple <Args...> &tpl)
{
	auto &em = Emitter::active;

	using T = std::decay_t <decltype(std::get <index> (tpl))>;
	using P = parameter_injector <T>;

	auto &x = std::get <index> (tpl);

	// Parameters in the target language
	if constexpr (P::value) {
		thunder::Index t = P::emit();
		thunder::Index q = em.emit_qualifier(t, index, thunder::parameter);
		thunder::Index c = em.emit_construct(q, -1, thunder::global);
		P::apply(x, c);
	}

	if constexpr (index + 1 < sizeof...(Args))
		inject_parameters <index + 1> (tpl);
}

} // namespace detail

// Internal construction of procedures
template <generic_or_void R, typename ... Args>
struct Procedure : thunder::TrackedBuffer {
	void call(std::tuple <Args...> &args) {
		if constexpr (sizeof...(Args))
			detail::inject_parameters <0> (args);
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

///////////////////////////////////
// Kickstart ordinary procedures //
///////////////////////////////////

template <generic_or_void R = void>
struct ProcedureBuilder {
	std::string name;

	template <detail::acceptable_callable F>
	requires (!std::same_as <typename detail::signature <F> ::return_t, void>)
	auto operator<<(F ftn) const {
		using S = detail::signature <F>;

		typename S::template manual_prodecure <R> result;

		auto &em = Emitter::active;

		em.push(result);
		{
			auto args = typename S::args_t();

			result.call(args);
			auto values = std::apply(ftn, args);
			_return(values);
		}
		em.pop();

		return result.named(name);
	}

	// Void functions are presumbed to contain necessary returns(...) statements already
	template <detail::acceptable_callable F>
	requires (std::same_as <typename detail::signature <F> ::return_t, void>)
	auto operator<<(F ftn) const {
		using S = detail::signature <F>;

		typename S::template manual_prodecure <R> result;
		
		auto &em = Emitter::active;

		em.push(result);
		{
			auto args = typename S::args_t();

			result.call(args);
			std::apply(ftn, args);
		}
		em.pop();

		return result.named(name);
	}
};

// Syntactic sugar
template <generic_or_void R, typename F>
struct manifest_skeleton {
	using proc = void;
};

template <generic_or_void R, typename ... Args>
struct manifest_skeleton <R, void (*)(Args...)> {
	using proc = Procedure <R, Args...>;
};

#define $subroutine(R, name, ...)							\
	::jvl::ire::manifest_skeleton <R, void (*)(__VA_ARGS__)> ::proc name		\
		= ::jvl::ire::ProcedureBuilder <R> (#name)				\
		<< [_returner = _return_igniter <R> ()](__VA_ARGS__) -> void

#define $entrypoint(name)								\
	::jvl::ire::Procedure <void> name						\
		= ::jvl::ire::ProcedureBuilder("main")					\
		<< [_returner = _return_igniter <void> ()]() -> void

} // namespace jvl::ire
