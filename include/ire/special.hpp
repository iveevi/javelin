#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// Task shader payload information
// TODO: generalize this structure for native/builtin/aggregate...
// TODO: to this end use the bound buffer object (rename)
template <generic T>
struct task_payload {};

template <builtin T>
struct task_payload <T> : T {
	template <typename ... Args>
	explicit task_payload(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		auto type = type_info_generator <T> (*this).synthesize();
		auto qual = em.emit_qualifier(type, -1, thunder::task_payload);
		auto value = em.emit_construct(qual, -1, thunder::transient);
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
		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, -1, thunder::task_payload);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		layout.link(value);
	}
};

// Raytracing payload
// TODO: move to intrinsics...
template <generic T>
struct ray_payload {};

template <builtin T>
struct ray_payload <T> : T {
	uint32_t binding = 0;

	template <typename ... Args>
	explicit ray_payload(uint32_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		this->ref = value;
	}

	ray_payload &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

template <aggregate T>
struct ray_payload <T> : T {
	uint32_t binding = 0;

	template <typename ... Args>
	explicit ray_payload(uint32_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;
		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		layout.link(value);
	}
};

template <generic T>
struct ray_payload_in {};

template <builtin T>
struct ray_payload_in <T> : T {
	uint32_t binding = 0;

	template <typename ... Args>
	explicit ray_payload_in(uint32_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload_in);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		this->ref = value;
	}

	ray_payload_in &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

template <aggregate T>
struct ray_payload_in <T> : T {
	uint32_t binding = 0;

	template <typename ... Args>
	explicit ray_payload_in(uint32_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;
		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload_in);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		layout.link(value);
	}
};

template <generic T>
struct hit_attribute {};

template <builtin T>
struct hit_attribute <T> : T {
	hit_attribute() : T() {
		synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto qual = em.emit_qualifier(type, -1, thunder::hit_attribute);
		auto dst = em.emit_construct(qual, -1, thunder::transient);
		// TODO: refactor to cached
		this->ref = dst;

		return cache_index_t::from(dst);
	}
};

template <aggregate T>
struct hit_attribute <T> : T {
	hit_attribute() : T() {
		synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		
		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, -1, thunder::hit_attribute);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		layout.link(value);

		return cache_index_t::from(value);
	}
};

// TODO: transient_qualified_wrapper
// then base all of this from that...

} // namespace jvl::ire