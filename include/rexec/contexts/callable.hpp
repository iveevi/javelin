#pragma once

#include "../class.hpp"
#include "../validation.hpp"

namespace jvl::rexec {

////////////////////
// Callable REXEC //
////////////////////

template <resource ... Resources>
struct Callable : ResourceExecutionContext <ExecutionFlag::eSubroutine, Resources...> {
	using self = Callable;

	// Nothing extra here...
	// can only import other callables
	
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

#define $rexec_callable(name, ...)	struct name : Callable <__VA_ARGS__>

} // namespace jvl::rexec