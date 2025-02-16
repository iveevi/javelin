#pragma once

#include "buffer_objects.hpp"

namespace jvl::ire {

// Shared variables need some unique tracking info (takes over the binding)
template <generic T>
struct shared : bound_buffer_object <T, thunder::shared> {
	static thunder::Index id() {
		static thunder::Index id = 0;
		return id++;
		// TODO: callbacks on scopes...
	}

	template <typename ... Args>
	shared(const Args &...args) : bound_buffer_object <T, thunder::shared> (id(), args...) {}
};

} // namespace jvl::ire