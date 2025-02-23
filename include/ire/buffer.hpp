#pragma once

#include "memory.hpp"
#include "qualified_wrapper.hpp"

namespace jvl::ire {

/////////////////////////////////
// Uniform and storage buffers //
/////////////////////////////////

template <generic T>
using uniform = qualified_wrapper <T, thunder::uniform_buffer>;

template <generic T>
using buffer = qualified_wrapper <T, thunder::storage_buffer>;

// Concept for buffer types
template <typename T>
struct is_buffer_like : std::false_type {};

template <generic T>
struct is_buffer_like <buffer <T>> : std::true_type {};

template <typename T>
concept buffer_like = generic <T> && is_buffer_like <T> ::value;

// Implementing qualifiers for buffer
template <buffer_like Buffer>
struct readonly <Buffer> : Buffer {
	template <typename ... Args>
	readonly(const Args &... args) : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index qualifier = em.emit_qualifier(Buffer::qualifier(), -1, thunder::readonly);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <buffer_like Buffer>
struct writeonly <Buffer> : Buffer {
	template <typename ... Args>
	writeonly(const Args &... args) : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index qualifier = em.emit_qualifier(Buffer::qualifier(), -1, thunder::writeonly);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <buffer_like Buffer>
struct scalar <Buffer> : Buffer {
	// Same qualifier process....
	thunder::Index qualifier() {
		auto &em = Emitter::active;
		return em.emit_qualifier(Buffer::qualifier(), -1, thunder::scalar);
	}

	// Builtin generics...
	template <typename ... Args>
	scalar(const Args &... args)
	requires builtin <Buffer> : Buffer(args...) {
		this->ref = synthesize();
	}

	cache_index_t synthesize() const
	requires builtin <Buffer> {
		auto &em = Emitter::active;
		thunder::Index value = em.emit_construct(qualifier(), -1, thunder::transient);
		return cache_index_t::from(value);
	}
	
	// Aggregate generics...
	template <typename ... Args>
	scalar(const Args &... args)
	requires aggregate <Buffer> : Buffer(args...) {
		auto &em = Emitter::active;
		thunder::Index value = em.emit_construct(qualifier(), -1, thunder::transient);
		this->layout().link(value);
	}
};

template <buffer_like T>
struct is_buffer_like <scalar <T>> : std::true_type {};

} // namespace jvl::ire