#pragma once

#include "../thunder/enumerations.hpp"
#include "concepts.hpp"
#include "memory.hpp"

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
		auto type = native_t <T> ::type();
		auto pc = em.emit_qualifier(type, binding, K);
		auto value = em.emit_construct(pc, -1, thunder::transient);
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
		auto value = em.emit_construct(qualifier(), -1, thunder::transient);
		this->ref = value;
	}

	thunder::Index qualifier() const {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();
		
		return em.emit_qualifier(type, binding, K);
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
		
		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto pc = em.emit_qualifier(type, binding, K);
		auto value = em.emit_construct(pc, -1, thunder::transient);

		this->layout().link(value);
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

// Concept for buffer types
template <typename T>
struct is_buffer_like : std::false_type {};

template <generic T>
struct is_buffer_like <buffer <T>> : std::true_type {};

template <typename T>
concept buffer_like = builtin <T> && is_buffer_like <T> ::value;

// Implementing qualifiers for buffer
template <buffer_like Buffer>
struct read_only <Buffer> : Buffer {
	template <typename ... Args>
	read_only(const Args &... args) : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index qualifier = em.emit_qualifier(Buffer::qualifier(), -1, thunder::read_only);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <buffer_like Buffer>
struct write_only <Buffer> : Buffer {
	template <typename ... Args>
	write_only(const Args &... args) : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index qualifier = em.emit_qualifier(Buffer::qualifier(), -1, thunder::write_only);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <buffer_like Buffer>
struct scalar <Buffer> : Buffer {
	template <typename ... Args>
	scalar(const Args &... args) : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index qualifier = em.emit_qualifier(Buffer::qualifier(), -1, thunder::scalar);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <buffer_like T>
struct is_buffer_like <scalar <T>> : std::true_type {};

} // namespace jvl::ire
