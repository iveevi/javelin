#pragma once

#include "../thunder/enumerations.hpp"
#include "emitter.hpp"
#include "native.hpp"
#include "tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

template <generic>
struct push_constant {};

// Implementation for native types
template <native T>
struct push_constant <T> {
	using arithmetic_type = native_t <T>;

	push_constant() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t lin = em.emit_qualifier(type, -1, thunder::push_constant);
		thunder::index_t value = em.emit_load(lin, -1);
		return cache_index_t::from(value);
	}

	operator arithmetic_type() const {
		return arithmetic_type(synthesize());
	}
};

// Implementation built-ins
template <builtin T>
struct push_constant <T> : T {
	push_constant() {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t lin = em.emit_qualifier(type, -1, thunder::push_constant);
		this->ref = lin;
	}
	
	operator typename T::arithmetic_type() const {
		return arithmetic_type(T::synthesize());
	}
};

// Implementation for aggregate types
template <aggregate T>
struct push_constant <T> : T {
	template <typename ... Args>
	push_constant(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::index_t type = type_field_from_args(layout).id;
		thunder::index_t lin = em.emit_qualifier(type, -1, thunder::push_constant);
		layout.ref_with(cache_index_t::from(lin));
	}
};

} // namespace jvl::ire
