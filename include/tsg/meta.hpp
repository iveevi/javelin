#pragma once

#include <type_traits>

#include "flags.hpp"

namespace jvl::tsg {

// Matching for template types of the form:
// (ShaderStageFlags, Type, Binding OR (non-template type parameters)...)
template <template <ShaderStageFlags, typename, auto ...> typename Meta, typename T>
struct is_stage_meta : std::false_type {};

template <template <ShaderStageFlags, typename, auto ...> typename Meta,
	ShaderStageFlags A,
	typename B,
	auto ... Args>
struct is_stage_meta <Meta, Meta <A, B, Args...>> : std::true_type {};

template <template <ShaderStageFlags, typename, auto ...> typename Meta, typename T>
constexpr bool is_stage_meta_v = is_stage_meta <Meta, T> ::value;

// Filtering types into a "container" template type
template <typename Container, typename ... Specifiers>
struct collector {
	// It is not valid to actually instantiate this one...
	static_assert(false);
};

} // namespace jvl::tsg