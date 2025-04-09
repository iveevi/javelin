#pragma once

#include "../../ire/intrinsics/glsl.hpp"

#include "../class.hpp"
#include "../validation.hpp"

namespace jvl::rexec {

//////////////////
// Vertex REXEC //
//////////////////

template <resource ... Resources>
struct Fragment : ResourceExecutionContext <ExecutionFlag::eFragment, Resources...> {
	using self = Fragment;

	// TODO: import the rest of the instrinsics here...

	// TODO: specify rasterization options here as well?

	template <rexec_class T>
	static T _use() {
		using collection = resource_collection <Resources...>;

		// Ensure execution compatibility
		static_assert(T::flag == ExecutionFlag::eSubroutine,
			"only subroutines are allowed as sub-RECs");

		// Ensure resource compatibility
		static_assert(resource_subset <collection, typename T::collection>,
			"imported scope uses resource not specified in the current REC");

		return T();
	}
};

// TODO: Implementing the intrinsics

//////////////
// Concepts //
//////////////

template <resource ... Resources>
bool fragment_pass(const Fragment <Resources...> &);

template <typename T>
concept fragment_class = requires(const T &value) {
	{ fragment_pass(value) } -> std::same_as <bool>;
};

template <typename T>
concept fragment_class_method = rexec_class_method <T> && fragment_class <typename T::rexec>;

template <typename T>
concept fragment_class_entrypoint = fragment_class_method <T> && ire::entrypoint_class <T>;

// Sugar for declaring Fragment REXEC contexts
#define $rexec_fragment(name, ...)	struct name : Fragment <__VA_ARGS__>

} // namespace jvl::rexec