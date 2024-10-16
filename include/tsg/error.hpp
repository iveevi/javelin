#pragma once

#include "requirement.hpp"

namespace jvl::tsg {

// Error stage for requirements
template <requirement ... Requirements>
struct requirement_error_report {};

template <requirement R, requirement ... Requirements>
struct requirement_error_report <R, Requirements...> {
	static constexpr auto next = requirement_error_report <Requirements...>();
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N>
struct dedicated_reporter {
	static_assert(false, "shader dependency is unsatisfied");
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N, requirement ... Requirements>
struct requirement_error_report <RequirementCompatibleLayout <S, T, F, N>, Requirements...> {
	static constexpr auto next = requirement_error_report <Requirements...> ();
	static constexpr auto reporter = dedicated_reporter <S, T, F, N> ();
};

template <typename T>
struct requirement_error_report_vector {};

template <requirement ... Requirements>
struct requirement_error_report_vector <RequirementVector <Requirements...>> {
	using type = requirement_error_report <Requirements...>;
};

// Status retrieval from requirement resolution
template <requirement R>
struct requirement_status {
	static constexpr bool status = true;
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N>
struct requirement_status <RequirementCompatibleLayout <S, T, F, N>> {
	static constexpr bool status = false;
};

template <requirement R, requirement ... Requirements>
constexpr bool verification_status()
{
	using reporter = requirement_status <R>;
	if constexpr (sizeof...(Requirements))
		return reporter::status && verification_status <Requirements...> ();

	return reporter::status;
}

template <requirement ... Requirements>
constexpr bool verification_status(const RequirementVector <Requirements...> &reqs)
{
	if constexpr (sizeof...(Requirements))
		return verification_status <Requirements...> ();

	return true;
}

} // namespace jvl::tsg