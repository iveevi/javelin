#pragma once

#include "../thunder/enumerations.hpp"
#include "concepts.hpp"

namespace jvl::ire {

template <generic, thunder::QualifierKind>
struct qualified_wrapper {};

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

	thunder::Index qualifier() {
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
		auto value = em.emit_construct(qualifier(), -1, thunder::transient);
		this->layout().link(value);
	}

	thunder::Index qualifier() {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();
		
		return em.emit_qualifier(type, binding, K);
	}
};

} // namespace jvl::ire
