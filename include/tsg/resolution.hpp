#pragma once

#include "outputs.hpp"
#include "requirement.hpp"

namespace jvl::tsg {

// Requirement resolution processor
template <requirement R, stage_output ... Outputs>
struct resolve_requirement {
	using result = R;
};

template <requirement R, stage_output O, stage_output ... Outputs>
struct resolve_requirement <R, O, Outputs...> {
	using result = resolve_requirement <R, Outputs...> ::result;
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N, stage_output ... Outputs>
struct resolve_requirement <RequirementCompatibleLayout <S, T, F, N>, StageOut <F, T, N>, Outputs...> {
	using result = RequirementSatisfied;
};

template <typename Rs, typename Os>
struct resolve_requirement_vector {};

template <requirement ... Requirements, stage_output ... Outputs>
struct resolve_requirement_vector <RequirementVector <Requirements...>, StageOutputs <Outputs...>> {
	using result = RequirementVector <typename resolve_requirement <Requirements, Outputs...> ::result...>;
};

} // namespace jvl::tsg