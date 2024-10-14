#pragma once

#include "../thunder/enumerations.hpp"
#include "emitter.hpp"
#include "native.hpp"
#include "tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

//////////////////////////////
// Interpolation qualifiers //
//////////////////////////////

enum InterpolationKind {
	flat,
	noperspective,
	smooth,
};

constexpr thunder::QualifierKind layout_in_as(InterpolationKind kind)
{
	switch (kind) {
	case flat:
		return thunder::layout_in_flat;
	case noperspective:
		return thunder::layout_in_noperspective;
	case smooth:
		return thunder::layout_in_smooth;
	}

	return thunder::basic;
}

constexpr thunder::QualifierKind layout_out_as(InterpolationKind kind)
{
	switch (kind) {
	case flat:
		return thunder::layout_out_flat;
	case noperspective:
		return thunder::layout_out_noperspective;
	case smooth:
		return thunder::layout_out_smooth;
	}

	return thunder::basic;
}

///////////////////
// Layout inputs //
///////////////////

template <generic, InterpolationKind = smooth>
struct layout_in {};

// Implementation for native types
template <native T, InterpolationKind kind>
struct layout_in <T, kind> {
	using arithmetic_type = native_t <T>;

	size_t binding;

	layout_in(size_t binding_ = 0) : binding(binding_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t lin = em.emit_qualifier(type, binding, layout_in_as(kind));
		thunder::index_t value = em.emit_construct(lin, -1, thunder::transient);
		return cache_index_t::from(value);
	}

	operator arithmetic_type() const {
		return arithmetic_type(synthesize());
	}
};

// Implementation for built-ins
template <builtin T, InterpolationKind kind>
struct layout_in <T, kind> : T {
	size_t binding;

	layout_in(size_t binding_ = 0) : binding(binding_) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t lin = em.emit_qualifier(type, binding, layout_in_as(kind));
		thunder::index_t value = em.emit_construct(lin, -1, thunder::transient);
		this->ref = value;
	}

	layout_in &operator=(const layout_in &other) {
		binding = other.binding;
		this->ref = other.ref;
		return *this;
	}
	
	operator typename T::arithmetic_type() const {
		return arithmetic_type(T::synthesize());
	}
};

// Implementation for aggregate types
template <aggregate T, InterpolationKind kind>
struct layout_in <T, kind> : T {
	size_t binding;

	template <typename ... Args>
	layout_in(size_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::index_t type = type_field_from_args(layout).id;
		thunder::index_t lin = em.emit_qualifier(type, binding, layout_in_as(kind));
		thunder::index_t value = em.emit_construct(lin, -1, thunder::transient);
		layout.ref_with(cache_index_t::from(value));
	}
};

////////////////////
// Layout outputs //
////////////////////

template <generic, InterpolationKind = smooth>
struct layout_out {
	static_assert(false, "aggregate layout outputs are not yet supported");
};

// Implementation for native types
template <native T, InterpolationKind kind>
struct layout_out <T, kind> {
	const size_t binding;

	layout_out(size_t binding_) : binding(binding_) {}

	layout_out &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t lout = em.emit_qualifier(type, binding, layout_out_as(kind));
		thunder::index_t dst = em.emit_construct(lout, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

// Implementation for built-ins
template <builtin T, InterpolationKind kind>
struct layout_out <T, kind> : T {
	size_t binding;

	layout_out(size_t binding_) : binding(binding_) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t lout = em.emit_qualifier(type, binding, layout_out_as(kind));
		thunder::index_t dst = em.emit_construct(lout, -1, thunder::transient);
		this->ref = dst;
	}

	layout_out &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

}
