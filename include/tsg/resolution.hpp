#pragma once

#include "requirement.hpp"

namespace jvl::tsg {

// Requirement resolution processor
template <requirement R, specifier ... Specifiers>
struct resolve_requirement {
	using result = R;
};

template <requirement R, specifier O, specifier ... Outputs>
struct resolve_requirement <R, O, Outputs...> {
	using result = resolve_requirement <R, Outputs...> ::result;
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N, specifier ... Outputs>
struct resolve_requirement <RequirementCompatibleLayout <S, T, F, N>, LayoutOut <F, T, N>, Outputs...> {
	using result = RequirementSatisfied;
};

template <ShaderStageFlags From, generic T, ShaderStageFlags To, size_t N, specifier ... Outputs>
struct resolve_requirement <RequirementOutputUsage <From, T, To, N>, LayoutIn <To, T, N>, Outputs...> {
	using result = RequirementSatisfied;
};

template <typename Rs, typename ... Args>
struct resolve_requirement_vector {};

template <requirement ... Requirements, specifier ... Outputs>
struct resolve_requirement_vector <RequirementVector <Requirements...>,  Outputs...> {
	using result = RequirementVector <typename resolve_requirement <Requirements, Outputs...> ::result...>;
};

} // namespace jvl::tsg