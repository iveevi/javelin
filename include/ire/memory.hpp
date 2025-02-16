#pragma once

#include "concepts.hpp"

namespace jvl::ire {

///////////////////////
// Memory qualifiers //
///////////////////////

template <builtin T>
struct coherent : T {
	static_assert(false, "coherent is not supported for given type");
};

template <builtin T>
struct restrict : T {
	static_assert(false, "restrict is not supported for given type");
};

template <builtin T>
struct read_only : T {
	static_assert(false, "read_only is not supported for given type");
};

template <builtin T>
struct write_only : T {
	static_assert(false, "write_only is not supported for given type");
};

} // namespace jvl::ire