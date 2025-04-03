#pragma once

#include "../ire/concepts.hpp"

namespace jvl::rexec {

// Importing definitions
using ire::generic;
using ire::generic_or_void;

//////////////////////////////
// Resource implementations //
//////////////////////////////

// Layout inputs
// TODO: interpolation modes...
struct _layout_in {};

template <generic T, size_t Location>
struct LayoutIn {
	static constexpr size_t location = Location;

	using check = _layout_in;
	using value_type = T;
};

template <typename T>
concept resource_layout_in = std::same_as <typename T::check, _layout_in>;

#define $layout_in(T, B, ...)	LayoutIn <T, B>

// Layout outputs
struct _layout_out {};

template <generic T, size_t Location>
struct LayoutOut {
	static constexpr size_t location = Location;

	using check = _layout_out;
	using value_type = T;
};

template <typename T>
concept resource_layout_out = std::same_as <typename T::check, _layout_out>;

#define $layout_out(T, B)	LayoutOut <T, B>

// Push constants
struct _push_constant {};

// TODO: should be solid_generic (as opposed to soft_generic)
template <generic T, size_t Offset>
struct PushConstant {
	static constexpr size_t offset = Offset;

	using check = _push_constant;
	using value_type = T;
};

template <typename T>
concept resource_push_constant = std::same_as <typename T::check, _push_constant>;

#define $push_constant(T, O)	PushConstant <T, O>

// Resource concept
template <typename T>
concept resource = false
	|| resource_layout_in <T>
	|| resource_layout_out <T>
	|| resource_push_constant <T>;

} // namespace jvl::rexec