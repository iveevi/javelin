#pragma once

#include "../../ire/intrinsics/glsl.hpp"

#include "../class.hpp"
#include "../validation.hpp"

namespace jvl::rexec {

//////////////////
// Vertex REXEC //
//////////////////

template <resource ... Resources>
struct Vertex : ResourceExecutionContext <ExecutionFlag::eVertex, Resources...> {
	using self = Vertex;

	static ire::gl_Position_t gl_Position;
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

// Implementing the intrinsics
template <resource ... Resources>
ire::gl_Position_t Vertex <Resources...> ::gl_Position;

//////////////
// Concepts //
//////////////

template <resource ... Resources>
bool vertex_pass(const Vertex <Resources...> &);

template <typename T>
concept vertex_class = requires(const T &value) {
	{ vertex_pass(value) } -> std::same_as <bool>;
};

template <typename T>
concept vertex_class_method = rexec_class_method <T> && vertex_class <typename T::rexec>;

template <typename T>
concept vertex_class_entrypoint = vertex_class_method <T> && ire::entrypoint_class <T>;

// Sugar for declaring Vertex REXEC contexts
#define $rexec_vertex(name, ...)	struct name : Vertex <__VA_ARGS__>

} // namespace jvl::rexec