#pragma once

#include "qualified_wrapper.hpp"

namespace jvl::ire {

// Shared variables need some unique tracking info (takes over the binding)
template <generic T>
struct shared : qualified_wrapper <T, thunder::shared> {
	static thunder::Index id() {
		static thunder::Index id = 0;
		return id++;
		// TODO: callbacks on scopes...
	}

	template <typename ... Args>
	shared(const Args &...args) : qualified_wrapper <T, thunder::shared> (id(), args...) {}
};

} // namespace jvl::ire