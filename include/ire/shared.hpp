#pragma once

#include "buffer_objects.hpp"

namespace jvl::ire {

// Shared variables need some unique tracking info (takes over the binding)
template <generic T>
struct shared : bound_buffer_object <T, thunder::shared> {
	static thunder::index_t id() {
		static thunder::index_t id = 0;
		return id++;
		// TODO: callbacks on scopes...
	}

	shared() : bound_buffer_object <T, thunder::shared> (id()) {}
};

} // namespace jvl::ire