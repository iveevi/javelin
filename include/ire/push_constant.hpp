#pragma once

#include "qualified_wrapper.hpp"

namespace jvl::ire {

// Bindless versions, for push constants
template <generic T>
struct push_constant : qualified_wrapper <T, thunder::push_constant> {
	template <typename ... Args>
	push_constant(size_t offset = 0, const Args &... args)
		: qualified_wrapper <T, thunder::push_constant> (offset, args...) {}

	push_constant(const push_constant &) = default;

	// Override the assignment operator (from arithmetic types) to be a no-op
	push_constant &operator=(const push_constant &) {
		return *this;
	}
};

} // namespace jvl::ire