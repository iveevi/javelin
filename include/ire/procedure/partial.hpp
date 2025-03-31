#pragma once

#include "ordinary.hpp"
#include "signature.hpp"
#include "type_checking.hpp"

namespace jvl::ire {

////////////////////////////////////////////
// Procedures with partial specialization //
////////////////////////////////////////////

template <generic_or_void R, generic_or_void RF, typename A, typename B>
struct PartialProcedureSpecialization {};

template <generic_or_void R, generic_or_void RF, typename ... CArgs, generic ... RArgs>
struct PartialProcedureSpecialization <R, RF, std::tuple <CArgs...>, std::tuple <RArgs...>> {
	std::string name;
	const std::function <RF (CArgs..., RArgs...)> &hold;

	auto operator()(CArgs ... cargs) {
		return ProcedureBuilder <R> (name) << [this, cargs...](RArgs ... rargs) -> RF {
			return hold(cargs..., rargs...);
		};
	}
};

template <generic_or_void R, generic_or_void RF, typename ... Args>
struct PartialProcedure {
	// R  -> intended return type (shader)
	// RT -> true return type of the invocable (c++)
	std::string name;
	std::function <RF (Args...)> hold;

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
				R, RF,
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

template <generic_or_void R, generic_or_void RF, typename ... Args>
auto build_partial_procedure(const std::string &name,
			     const detail::type_packet <Args...> &,
			     const std::function <RF (Args...)> &passed)
{
	return PartialProcedure <R, RF, Args...> (name, passed);
}

template <generic_or_void R>
struct PartialProcedureBuilder {
	std::string name;

	template <detail::acceptable_callable F>
	auto operator<<(const F &passed) const {
		using S = detail::signature <F>;
		using packet = typename S::packet;

		return build_partial_procedure <R, typename S::return_t>
			(name, packet(), std::function(passed));
	}
};

// Short-hand macro for writing specializable shader functions
#define $partial_subroutine(R, name)	auto name = ::jvl::ire::PartialProcedureBuilder <R> (#name) << []

#define $partial_entrypoint(name)	auto name = ::jvl::ire::PartialProcedureBuilder <void> ("main") << []

} // namespace jvl::ire