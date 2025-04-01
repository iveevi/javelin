#pragma once

#include "ordinary.hpp"
#include "signature.hpp"
#include "type_checking.hpp"

namespace jvl::ire {

////////////////////////////////////////////
// Procedures with partial specialization //
////////////////////////////////////////////

template <generic_or_void R, typename A, typename B>
struct PartialProcedureSpecialization {};

template <generic_or_void R, typename ... CArgs, generic ... RArgs>
struct PartialProcedureSpecialization <R, std::tuple <CArgs...>, std::tuple <RArgs...>> {
	std::string name;
	const std::function <void (CArgs..., RArgs...)> &hold;

	auto operator()(CArgs ... cargs) {
		return ProcedureBuilder <R> (name) << [this, cargs...](RArgs ... rargs) -> void {
			return hold(cargs..., rargs...);
		};
	}
};

template <generic_or_void R, typename ... Args>
struct PartialProcedure {
	// R  -> intended return type (shader)
	// RT -> true return type of the invocable (c++)
	std::string name;
	std::function <void (Args...)> hold;

	template <typename ... TArgs>
	// TODO: require non generic constants...
	auto operator()(TArgs ... args) {
		using args_t = std::tuple <Args...>;
		using targs_t = std::tuple <TArgs...>;
		using slicer_eval_t = is_type_slice <targs_t, args_t>;
		using slicer_remainder_t = typename slicer_eval_t::remainder;

		// Type checking (only one error will pop up at a time...)
		static_assert(slicer_eval_t::value,
			"invalid partial procedure specialization: "
			"mismatching types");
		static_assert(!slicer_eval_t::value || slicer_eval_t::generics,
			"invalid partial procedure specialization: "
			"remaining arguments must be (JVL) generic");
		
		if constexpr (slicer_eval_t::value && slicer_eval_t::generics) {
			return PartialProcedureSpecialization <
				R,
				targs_t,
				slicer_remainder_t
			> (name, hold)(args...);
		} else {
			// Return nothing, which results in errors downstream
			return;
		}
	}
};

/////////////////////////////////////////////////
// Kickstart partial specialization procedures //
/////////////////////////////////////////////////

template <generic_or_void R, typename ... Args>
auto build_partial_procedure(const std::string &name,
			     const detail::type_packet <Args...> &,
			     const std::function <void (Args...)> &passed)
{
	return PartialProcedure <R, Args...> (name, passed);
}

template <generic_or_void R>
struct PartialProcedureBuilder {
	std::string name;

	template <detail::acceptable_callable F>
	auto operator<<(const F &passed) const {
		using S = detail::signature <F>;
		using packet = typename S::packet;

		return build_partial_procedure <R> (name, packet(), std::function(passed));
	}
};

// Short-hand macro for writing specializable shader functions
template <generic_or_void R, typename F>
struct manifest_partial_skeleton {
	using proc = void;
};

template <generic_or_void R, typename ... Args>
struct manifest_partial_skeleton <R, void (*)(Args...)> {
	using proc = PartialProcedure <R, Args...>;
};

#define $partial_subroutine(R, name, ...)							\
	::jvl::ire::manifest_partial_skeleton <R, void (*)(__VA_ARGS__)> ::proc name		\
		= ::jvl::ire::PartialProcedureBuilder <R> (#name) << [](__VA_ARGS__) -> void

#define $partial_entrypoint(name)	auto name = ::jvl::ire::PartialProcedureBuilder <void> ("main") << []

} // namespace jvl::ire