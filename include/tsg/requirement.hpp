#pragma once

#include "flags.hpp"
#include "meta.hpp"
#include "imports.hpp"
#include "artifact.hpp"

namespace jvl::tsg {

// Filtering stage requirements (transformed)
template <ShaderStageFlags RequiredBy, generic Type, ShaderStageFlags RequiredFrom, size_t Binding>
struct RequirementCompatibleLayout {};

// For later checks to see if an output layout is used
template <ShaderStageFlags From, generic Type, ShaderStageFlags To, size_t Binding>
struct RequirementOutputUsage {};

// Post-resolution stage requirement "variants"
struct RequirementSatisfied {};
struct RequirementNull {};

template <typename T>
concept requirement = is_stage_meta_v <RequirementCompatibleLayout, T>
	|| is_stage_meta_v <RequirementOutputUsage, T>
	|| std::is_same_v <T, RequirementSatisfied>
	|| std::is_same_v <T, RequirementNull>;

// Requirement vector with associated operations
template <requirement ... Requirements>
struct RequirementVector {
	using filtered = RequirementVector <Requirements...>;

	template <requirement R>
	using inserted = RequirementVector <R, Requirements...>;
};

template <requirement R, requirement ... Requirements>
struct RequirementVector <R, Requirements...> {
	using previous = RequirementVector <Requirements...> ::filtered;
	using filtered = previous::template inserted <R>;

	template <requirement Rs>
	using inserted = RequirementVector <Rs, R, Requirements...>;
};

template <requirement ... Requirements>
struct RequirementVector <RequirementNull, Requirements...> {
	using filtered = RequirementVector <Requirements...> ::filtered;

	template <requirement R>
	using inserted = RequirementVector <R, RequirementNull, Requirements...>;
};

// Requirement conversion from specifiers
template <specifier S>
struct requirement_for {
	using type = RequirementNull;
};

template <generic T, size_t N>
struct requirement_for <LayoutIn <ShaderStageFlags::eFragment, T, N>> {
	using type = RequirementCompatibleLayout <ShaderStageFlags::eFragment, T,
						  ShaderStageFlags::eVertex, N>;
};

template <generic T, size_t N>
struct requirement_for <LayoutOut <ShaderStageFlags::eVertex, T, N>> {
	using type = RequirementOutputUsage <ShaderStageFlags::eVertex, T,
					     ShaderStageFlags::eFragment, N>;
};

} // namespace jvl::tsg