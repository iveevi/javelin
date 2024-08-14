#pragma once

#include "emitter.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "uniform_layout.hpp"
#include "type_synthesis.hpp"
#include "upcast.hpp"

namespace jvl::ire {

struct __empty {};

template <typename T>
using layout_in_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, tagged>;

template <typename T>
using layout_out_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, __empty>;

template <global_qualifier_compatible T, size_t binding>
struct layout_in : layout_in_base <T> {
	using upcast_t = decltype(upcast(T()));

	layout_in()
	requires primitive_type <T> = default;

	layout_in()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_in(const Args &... args)
	requires uniform_compatible <T> : T(args...) {
		auto &em = Emitter::active;

		auto layout = this->layout();

		atom::Global global;
		global.type = type_field_from_args(layout).id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		cache_index_t ref;
		ref = em.emit(global);
		layout.__ref_with(ref);
	}

	cache_index_t synthesize() const
	requires primitive_type <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		atom::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit_main(load));
	}

	operator upcast_t()
	requires synthesizable <T> {
		return upcast_t(synthesize());
	}

	operator upcast_t ()
	requires primitive_type <T> {
		return upcast_t(synthesize());
	}
};

template <typename T, size_t binding>
requires primitive_type <T> || synthesizable <T> || uniform_compatible <T>
struct layout_out : layout_out_base <T> {
	using upcast_t = decltype(upcast(T()));

	layout_out()
	requires primitive_type <T> = default;

	layout_out()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_out(const Args &... args)
	requires uniform_compatible <T> : T(args...) {
		auto &em = Emitter::active;

		auto layout = this->layout();

		atom::Global global;
		global.type = type_field_from_args(layout).id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		cache_index_t ref;
		ref = em.emit(global);
		layout.__ref_with(ref);
	}

	layout_out &operator=(const T &t)
	requires primitive_type <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		atom::Store store;
		store.dst = em.emit(global);
		store.src = translate_primitive(t);

		em.emit_main(store);

		return *this;
	}

	layout_out &operator=(const upcast_t &t) {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		atom::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit_main(store);

		return *this;
	}
};

}
