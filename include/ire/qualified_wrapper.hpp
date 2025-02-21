#pragma once

#include "../thunder/enumerations.hpp"
#include "concepts.hpp"

namespace jvl::ire {

template <generic, thunder::QualifierKind>
struct qualified_wrapper {};

// Implementation for native types
template <native T, thunder::QualifierKind K>
struct qualified_wrapper <T, K> {
	using arithmetic_type = native_t <T>;
	
	size_t binding;

	qualified_wrapper(size_t binding_) :  binding(binding_) {}

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
struct qualified_wrapper <T, K> : T {
	size_t binding;

	template <typename ... Args>
	qualified_wrapper(size_t binding_, const Args &... args)
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
struct qualified_wrapper <T, K> : T {
	size_t binding;

	template <typename ... Args>
	qualified_wrapper(size_t binding_, const Args &... args)
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

} // namespace jvl::ire
