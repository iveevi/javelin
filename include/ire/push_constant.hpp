#pragma once

#include "qualified_wrapper.hpp"

namespace jvl::ire {

// Bindless versions, for push constants
template <typename T>
using push_constant_base = qualified_wrapper <T, thunder::push_constant, nullptr>;

template <generic T>
struct push_constant : push_constant_base <T> {
	template <typename ... Args>
	push_constant(size_t offset = 0, const Args &... args) : push_constant_base <T> (offset, args...) {}

	push_constant(const push_constant &) = default;

	// Override the assignment operator (from arithmetic types) to be a no-op
	push_constant &operator=(const push_constant &) {
		return *this;
	}
};

} // namespace jvl::ire