#pragma once

#include "meta.hpp"
#include "imports.hpp"

namespace jvl::tsg {

// Filtering out stage outputs
template <ShaderStageFlags F, generic T, size_t N>
struct StageOut {};

template <typename T>
concept stage_output = is_stage_meta_v <StageOut, T>;

template <stage_output ... Outputs>
struct StageOutputs {};

template <stage_output ... Outputs, specifier ... Specifiers>
struct collector <StageOutputs <Outputs...>, Specifiers...> {
	using result = StageOutputs <Outputs...>;
};

template <stage_output ... Outputs, specifier S, specifier ... Specifiers>
struct collector <StageOutputs <Outputs...>, S, Specifiers...> {
	using current = StageOutputs <Outputs...>;
	using result = typename collector <current, Specifiers...> ::result;
};

template <stage_output ... Outputs, ShaderStageFlags F, generic T, size_t N, specifier ... Specifiers>
struct collector <StageOutputs <Outputs...>, LayoutOut <F, T, N>, Specifiers...> {
	using current = StageOutputs <Outputs..., StageOut <F, T, N>>;
	using result = typename collector <current, Specifiers...> ::result;
};

} // namespace jvl::tsg