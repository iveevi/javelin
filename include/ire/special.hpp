#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// Task shader payload information
// TODO: generalize this structure for native/builtin/aggregate...
// TODO: to this end use the bound buffer object (rename)
template <generic T>
struct task_payload {};

template <native T>
struct task_payload <T> {
	task_payload &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		auto type = native_t <T> ::type();
		auto qual = em.emit_qualifier(type, -1, thunder::task_payload);
		auto dst = em.emit_construct(qual, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

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
template <generic T>
struct ray_payload {};

template <native T>
struct ray_payload <T> {
	uint32_t binding = 0;

	ray_payload(uint32_t binding_) : binding(binding_) {}

	ray_payload &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		auto type = native_t <T> ::type();
		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload);
		auto dst = em.emit_construct(qual, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

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

template <native T>
struct ray_payload_in <T> {
	uint32_t binding = 0;

	ray_payload_in(uint32_t binding_) : binding(binding_) {}

	ray_payload_in &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		auto type = native_t <T> ::type();
		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload_in);
		auto dst = em.emit_construct(qual, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

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
		auto layout = this->layout().remove_const();
		auto type = type_field_from_args(layout).id;
		auto qual = em.emit_qualifier(type, binding, thunder::ray_tracing_payload_in);
		auto value = em.emit_construct(qual, -1, thunder::transient);
		layout.ref_with(cache_index_t::from(value));
	}
};

template <generic T>
struct hit_attribute {};

template <native T>
struct hit_attribute <T> {
	hit_attribute() : T() {
		synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto type = native_t <T> ::type();
		auto qual = em.emit_qualifier(type, -1, thunder::hit_attribute);
		auto dst = em.emit_construct(qual, -1, thunder::transient);
		return cache_index_t::from(dst);
	}
};

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

} // namespace jvl::ire