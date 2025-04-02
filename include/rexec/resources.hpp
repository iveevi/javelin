#pragma once

#include "../ire/concepts.hpp"

namespace jvl::rexec {

// Importing definitions
using ire::generic;

//////////////////////////////
// Resource implementations //
//////////////////////////////

/*
TODO: something like

template <...>
struct T {
	using self = Check <T>;
};

concept = std::same_as <T::self, Check <T>>;
*/

#define DEFINE_RESOURCE_N2(name, concept_suffix, Ta, Tb)			\
	template <Ta A, Tb B>							\
	struct name {};								\
										\
	template <typename T>							\
	struct is_##concept_suffix : std::false_type {};			\
										\
	template <Ta A, Tb B>							\
	struct is_##concept_suffix <name <A, B>> : std::true_type {};		\
										\
	template <typename T>							\
	concept resource_##concept_suffix = is_##concept_suffix <T> ::value;

// Layout inputs
// TODO: interpolation modes...
DEFINE_RESOURCE_N2(LayoutIn, layout_in,
	generic,	// value type
	size_t		// binding
)

#define $layout_in(T, B, ...)	LayoutIn <T, B>

// Layout outputs
DEFINE_RESOURCE_N2(LayoutOut, layout_out,
	generic,	// value type
	size_t		// binding
)
#define $layout_out(T, B)	LayoutOut <T, B>

// Push constants
DEFINE_RESOURCE_N2(PushConstant, push_constant,
	generic,	// value type
	size_t		// offset
)
#define $push_constant(T, O)	PushConstant <T, O>

// Resource concept
template <typename T>
concept resource = false
	|| resource_layout_in <T>
	|| resource_layout_out <T>
	|| resource_push_constant <T>;

} // namespace jvl::rexec