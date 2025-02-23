#pragma once

#include "concepts.hpp"

namespace jvl::ire {

///////////////////////
// Memory qualifiers //
///////////////////////

template <generic T>
struct coherent : T {
	static_assert(false, "coherent is not supported for given type");
};

template <generic T>
struct restrict : T {
	static_assert(false, "restrict is not supported for given type");
};

template <generic T>
struct readonly : T {
	static_assert(false, "read_only is not supported for given type");
};

template <generic T>
struct writeonly : T {
	static_assert(false, "write_only is not supported for given type");
};

template <generic T>
struct scalar : T {
	static_assert(false, "scalar is not supported for given type");
};

} // namespace jvl::ire