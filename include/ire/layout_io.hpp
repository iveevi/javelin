#pragma once

#include "../thunder/enumerations.hpp"
#include "../thunder/qualified_type.hpp"
#include "emitter.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"
#include "upcast.hpp"

namespace jvl::ire {

// Dummy structure
// TODO: remove
struct __empty {};

// Interpolation qualifiers for layout outputs
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
}

// Base type for each layout
template <typename T>
using layout_in_base = std::conditional_t <builtin <T> || aggregate <T>, T, tagged>;

template <typename T>
using layout_out_base = std::conditional_t <builtin <T> || aggregate <T>, T, __empty>;

template <generic T, InterpolationKind kind = smooth>
struct layout_in : layout_in_base <T> {
	using upcast_t = decltype(upcast(T()));

	const size_t binding;

	layout_in(size_t binding_)
	requires native <T> : binding(binding_) {}

	layout_in(size_t binding_)
	requires builtin <T> : binding(binding_) {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.numerical = binding;
		global.kind = layout_in_as(kind);

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_in(size_t binding_, const Args &... args)
	requires native <T> : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Qualifier global;
		global.underlying = type_field_from_args(layout).id;
		global.numerical = binding;
		global.kind = layout_in_as(kind);

		cache_index_t ref;
		ref = em.emit(global);
		layout.ref_with(ref);
	}

	cache_index_t synthesize() const
	requires native <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.numerical = binding;
		global.kind = layout_in_as(kind);

		thunder::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit(load));
	}

	void refresh(const cache_index_t::value_type &) {
		// Do nothing for now...
	}

	operator upcast_t() const
	requires builtin <T> {
		return upcast_t(synthesize());
	}

	operator upcast_t () const
	requires native <T> {
		return upcast_t(synthesize());
	}
};

template <generic T, InterpolationKind kind = smooth>
struct layout_out : layout_out_base <T> {
	using upcast_t = decltype(upcast(T()));

	size_t binding;

	layout_out(size_t binding_)
	requires native <T> : binding(binding_) {}

	layout_out(size_t binding_)
	requires builtin <T> : binding(binding_) {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.numerical = binding;
		global.kind = layout_out_as(kind);

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_out(size_t binding_, const Args &... args)
	requires native <T> : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Qualifier global;
		global.underlying = type_field_from_args(layout).id;
		global.numerical = binding;
		global.kind = layout_out_as(kind);

		cache_index_t ref;
		ref = em.emit(global);
		layout.ref_with(ref);
	}

	layout_out &operator=(const T &t)
	requires native <T> {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.numerical = binding;
		global.kind = layout_out_as(kind);

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = translate_primitive(t);

		em.emit(store);

		return *this;
	}

	layout_out &operator=(const upcast_t &t) {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.numerical = binding;
		global.kind = layout_out_as(kind);

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit(store);

		return *this;
	}
};

}
