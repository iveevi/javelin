#pragma once

#include "../ire/layout_io.hpp"
#include "../ire/push_constant.hpp"

#include "collection.hpp"

namespace jvl::rexec {

#define DEFINE_RESOURCE_MANIFESTION(concept_suffix)						\
	template <typename T>									\
	struct resource_manifestion_wrapper_##concept_suffix					\
			: resource_manifestion_##concept_suffix <T> {				\
		using concept_suffix##s = T;							\
	};											\
												\
	template <resource ... Resources>							\
	using manifest_##concept_suffix = resource_manifestion_wrapper_##concept_suffix <	\
		typename filter_##concept_suffix <						\
			resource_collection <Resources...>					\
		> ::group									\
	>;

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
	static void _initializer() {}

	template <size_t B>
	static void layout_in() {
		static_assert(false, "invalid binding location for layout input");
	}
};

template <ire::generic T, size_t B, resource_layout_in ... Inputs>
struct resource_manifestion_layout_in <resource_collection <LayoutIn <T, B>, Inputs...>> {
	using next = resource_manifestion_layout_in <resource_collection <Inputs...>>;

	static void _initializer() {
		ire::layout_in <T> lin(B);
		return next::_initializer();
	}

	template <size_t I>
	static auto layout_in() {
		if constexpr (I == B)
			return ire::layout_in <T> (B);
		else
			return next::template layout_in <I> ();
	}
};

DEFINE_RESOURCE_MANIFESTION(layout_in)

// Layout outputs
template <typename T>
struct resource_manifestion_layout_out {
	static_assert(false, "invalid manifestion of layout outputs");
};

// TODO: validation to prevent conflicts (validation.hpp)
template <>
struct resource_manifestion_layout_out <resource_collection <>> {
	static void _initializer() {}

	template <size_t B>
	static void layout_out() {
		static_assert(false, "invalid binding location for layout output");
	}
};

template <ire::generic T, size_t B, resource_layout_out ... Outputs>
struct resource_manifestion_layout_out <resource_collection <LayoutOut <T, B>, Outputs...>> {
	using next = resource_manifestion_layout_out <resource_collection <Outputs...>>;

	static void _initializer() {
		ire::layout_out <T> lout(B);
		return next::_initializer();
	}

	template <size_t I>
	static auto layout_out() {
		if constexpr (I == B)
			return ire::layout_out <T> (B);
		else
			return next::template layout_out <I> ();
	}
};

DEFINE_RESOURCE_MANIFESTION(layout_out)

// Push constants
template <typename T>
struct resource_manifestion_push_constant {
	static_assert(false, "invalid manifestion of push constants");
};

template <>
struct resource_manifestion_push_constant <resource_collection <>> {
	// Having zero push constants is OK,
	// there simply is no push constant available :)

	static void _initializer() {}
};

template <ire::generic T, size_t Offset>
struct resource_manifestion_push_constant <resource_collection <PushConstant <T, Offset>>> {
	static void _initializer() {
		ire::push_constant <T> pc(Offset);
	}

	static auto push_constant() {
		return ire::push_constant <T> (Offset);
	}
};

template <resource_push_constant ... Resources>
struct resource_manifestion_push_constant <resource_collection <Resources...>> {
	static_assert(false, "only one push constant is allowed per context");
};

DEFINE_RESOURCE_MANIFESTION(push_constant)

// Full resource manifestation
template <resource ... Resources>
struct resource_manifestion :
	manifest_layout_in <Resources...>,
	manifest_layout_out <Resources...>,
	manifest_push_constant <Resources...>
{
	// TODO: resource conflict verification...

	struct _initializer {
		template <typename ... Args>
		auto operator*(const std::function <void (Args...)> &lambda) const {
			return [=](Args ... args) {
				manifest_layout_in <Resources...> ::_initializer();
				manifest_layout_out <Resources...> ::_initializer();
				manifest_push_constant <Resources...> ::_initializer();
				return lambda(args...);
			};
		}
		
		template <typename F>
		auto operator*(const F &lambda) const {
			return operator*(std::function(lambda));
		}
	};
};

// Sugared accessors for resources (requries self defined somewhere)
#define $lin(I)		self::layout_in <I> ()
#define $lout(I)	self::layout_out <I> ()
#define $constants	self::push_constant()

} // namespace jvl::rexec