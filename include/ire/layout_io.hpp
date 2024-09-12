#pragma once

#include "emitter.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "uniform_layout.hpp"
#include "type_synthesis.hpp"
#include "upcast.hpp"
#include <cstddef>

namespace jvl::ire {

struct __empty {};

template <typename T>
using layout_in_base = std::conditional_t <builtin <T> || aggregate <T>, T, tagged>;

template <typename T>
using layout_out_base = std::conditional_t <builtin <T> || aggregate <T>, T, __empty>;

template <generic T>
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
		global.kind = thunder::QualifierKind::layout_in;

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
		global.kind = thunder::QualifierKind::layout_in;

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
		global.kind = thunder::QualifierKind::layout_in;

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

template <generic T>
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
		global.kind = thunder::QualifierKind::layout_out;

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
		global.kind = thunder::QualifierKind::layout_out;

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
		global.kind = thunder::QualifierKind::layout_out;

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
		global.kind = thunder::QualifierKind::layout_out;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit(store);

		return *this;
	}
};

}
