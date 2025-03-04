#pragma once

#include "../thunder/enumerations.hpp"
#include "concepts.hpp"

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

// Implementation for built-ins
template <builtin T, InterpolationKind kind>
struct layout_in <T, kind> : T {
	size_t binding;

	explicit layout_in(size_t binding_ = 0) : binding(binding_) {
		auto &em = Emitter::active;
		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto lin = em.emit_qualifier(type, binding, layout_in_as(kind));
		auto value = em.emit_construct(lin, -1, thunder::global);
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
	explicit layout_in(size_t binding_, const Args &... args) : T(args...), binding(binding_) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::Index type = type_field_from_args(layout).id;
		thunder::Index lin = em.emit_qualifier(type, binding, layout_in_as(kind));
		thunder::Index value = em.emit_construct(lin, -1, thunder::global);
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

// Implementation for built-ins
template <builtin T, InterpolationKind kind>
struct layout_out <T, kind> : T {
	size_t binding;

	explicit layout_out(size_t binding_) : binding(binding_) {
		auto &em = Emitter::active;
		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();
			
		auto lout = em.emit_qualifier(type, binding, layout_out_as(kind));
		auto dst = em.emit_construct(lout, -1, thunder::global);
		this->ref = dst;
	}

	layout_out &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

} // namespace jvl::ire