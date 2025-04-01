#pragma once

#include "../ire/layout_io.hpp"
#include "../ire/push_constant.hpp"

#include "collection.hpp"

namespace jvl::rexec {

////////////////////////////////////////////
// Resource collection method manifestion //
////////////////////////////////////////////

// Layout inputs
template <typename T>
struct resource_manifestion_layout_in {
	static_assert(false, "invalid manifestion of layout inputs");
};

// TODO: validation to prevent conflicts (validation.hpp)
template <>
struct resource_manifestion_layout_in <resource_collection <>> {
	template <size_t B>
	static void layout_in() {
		static_assert(false, "invalid binding location for layout input");
	}
};

template <generic T, size_t B, resource_layout_in ... Inputs>
struct resource_manifestion_layout_in <resource_collection <LayoutIn <T, B>, Inputs...>> {
	using next = resource_manifestion_layout_in <resource_collection <Inputs...>>;

	template <size_t I>
	static auto layout_in() {
		if constexpr (I == B)
			return ire::layout_in <T> (B);
		else
			return next::template layout_in <I> ();
	}
};

template <resource ... Resources>
using manifest_layout_in = resource_manifestion_layout_in <
	typename filter_layout_in <
		resource_collection <Resources...>
	> ::group
>;

// Push constants
template <typename T>
struct resource_manifestion_push_constant {
	static_assert(false, "invalid manifestion of push constants");
};

template <generic T, size_t Offset>
struct resource_manifestion_push_constant <resource_collection <PushConstant <T, Offset>>> {
	static auto push_constant() {
		return ire::push_constant <T> (Offset);
	}
};

template <resource_push_constant ... Resources>
struct resource_manifestion_push_constant <resource_collection <Resources...>> {
	static_assert(false, "only one push constant is allowed per context");
};

// TODO: macro define_manifestation
template <resource ... Resources>
using manifest_push_constant = resource_manifestion_push_constant <
	typename filter_push_constant <
		resource_collection <Resources...>
	> ::group
>;

// Full resource manifestation
template <resource ... Resources>
struct resource_manifestion :
	manifest_layout_in <Resources...>,
	manifest_push_constant <Resources...>
	{};

// Sugared accessors for resources
#define $lin(I)		layout_in <I> ()
#define $lout(I)	layout_out <I> ()
#define $constants	push_constant()

} // namespace jvl::rexec