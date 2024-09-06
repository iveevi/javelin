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
using layout_in_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, tagged>;

template <typename T>
using layout_out_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, __empty>;

template <global_qualifier_compatible T>
struct layout_in : layout_in_base <T> {
	using upcast_t = decltype(upcast(T()));

	const size_t binding;

	layout_in(size_t binding_)
	requires primitive_type <T> : binding(binding_) {}

	layout_in(size_t binding_)
	requires synthesizable <T> : binding(binding_) {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_in;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_in(size_t binding_, const Args &... args)
	requires uniform_compatible <T> : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Global global;
		global.type = type_field_from_args(layout).id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_in;

		cache_index_t ref;
		ref = em.emit(global);
		layout.__ref_with(ref);
	}

	cache_index_t synthesize() const
	requires primitive_type <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_in;

		thunder::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit(load));
	}

	void refresh(const cache_index_t::value_type &) {
		// Do nothing for now...
	}

	operator upcast_t() const
	requires synthesizable <T> {
		return upcast_t(synthesize());
	}

	operator upcast_t () const
	requires primitive_type <T> {
		return upcast_t(synthesize());
	}
};

template <typename T>
requires primitive_type <T> || synthesizable <T> || uniform_compatible <T>
struct layout_out : layout_out_base <T> {
	using upcast_t = decltype(upcast(T()));

	size_t binding;

	layout_out(size_t binding_)
	requires primitive_type <T> : binding(binding_) {}

	layout_out(size_t binding_)
	requires synthesizable <T> : binding(binding_) {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_out;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_out(size_t binding_, const Args &... args)
	requires uniform_compatible <T> : T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Global global;
		global.type = type_field_from_args(layout).id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_out;

		cache_index_t ref;
		ref = em.emit(global);
		layout.__ref_with(ref);
	}

	layout_out &operator=(const T &t)
	requires primitive_type <T> {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_out;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = translate_primitive(t);

		em.emit(store);

		return *this;
	}

	layout_out &operator=(const upcast_t &t) {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = thunder::GlobalQualifier::layout_out;

		thunder::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit(store);

		return *this;
	}
};

}
