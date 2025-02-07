#pragma once

#include "type_system.hpp"

namespace jvl::ire {

// Task shader payload information
// TODO: generalize this structure for native/builtin/aggregate...
template <generic T>
struct task_payload {};

template <native T>
struct task_payload <T> {
	task_payload &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t dst = em.emit_construct(qual, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

template <builtin T>
struct task_payload <T> : T {
	template <typename ... Args>
	explicit task_payload(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t value = em.emit_construct(qual, -1, thunder::transient);
		this->ref = value;
	}

	task_payload &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

template <aggregate T>
struct task_payload <T> : T {
	template <typename ... Args>
	explicit task_payload(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::index_t type = type_field_from_args(layout).id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t value = em.emit_construct(qual, -1, thunder::transient);
		layout.ref_with(cache_index_t::from(value));
	}
};

}