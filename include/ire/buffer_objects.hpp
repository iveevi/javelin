#pragma once

#include "../thunder/enumerations.hpp"
#include "emitter.hpp"
#include "native.hpp"
#include "tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

template <generic, thunder::QualifierKind>
struct bound_buffer_object {};

// Implementation for native types
template <native T, thunder::QualifierKind K>
struct bound_buffer_object <T, K> {
	using arithmetic_type = native_t <T>;
	
	size_t binding;

	bound_buffer_object(size_t binding_) :  binding(binding_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t pc = em.emit_qualifier(type, binding, K);
		thunder::index_t value = em.emit_construct(pc, -1, thunder::transient);
		return cache_index_t::from(value);
	}

	operator arithmetic_type() const {
		return arithmetic_type(synthesize());
	}
};

// Implementation built-ins
template <builtin T, thunder::QualifierKind K>
struct bound_buffer_object <T, K> : T {
	size_t binding;

	template <typename ... Args>
	bound_buffer_object(size_t binding_, const Args &... args)
			: T(args...), binding(binding_) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t pc = em.emit_qualifier(type, binding, K);
		thunder::index_t value = em.emit_construct(pc, -1, thunder::transient);
		this->ref = value;
	}
	
	operator typename T::arithmetic_type() const {
		return arithmetic_type(T::synthesize());
	}
};

// Implementation for aggregate types
template <aggregate T, thunder::QualifierKind K>
struct bound_buffer_object <T, K> : T {
	size_t binding;

	template <typename ... Args>
	bound_buffer_object(size_t binding_, const Args &... args)
			: T(args...), binding(binding_) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t pc = em.emit_qualifier(type, binding, K);
		thunder::index_t value = em.emit_construct(pc, -1, thunder::transient);
		layout.ref_with(cache_index_t::from(value));
	}
};

// Bindless versions, for push constants
template <generic T>
struct push_constant : bound_buffer_object <T, thunder::push_constant> {
	template <typename ... Args>
	push_constant(size_t offset = 0, const Args &... args)
			: bound_buffer_object <T, thunder::push_constant> (offset, args...) {}

	push_constant(const push_constant &) = default;

	// Override the assignment operator (from arithmetic types) to be a no-op
	push_constant &operator=(const push_constant &) {
		return *this;
	}
};

// Specializations
template <generic T>
using uniform = bound_buffer_object <T, thunder::uniform>;

template <generic T>
using buffer = bound_buffer_object <T, thunder::storage_buffer>;

template <generic T>
using read_only_buffer = bound_buffer_object <T, thunder::read_only_storage_buffer>;

template <generic T>
using write_only_buffer = bound_buffer_object <T, thunder::write_only_storage_buffer>;

} // namespace jvl::ire
