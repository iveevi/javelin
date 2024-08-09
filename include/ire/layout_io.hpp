#pragma once

#include "emitter.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "uniform_layout.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

// TODO: gl_type -> jvl_primitive_t
// TODO: gltype_complatible -> is_jvl_primitive
// synthesizable -> is_synthesizable
// uniform_compatible -> is_uniform_compatible

struct __empty {};

template <typename T>
using layout_in_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, tagged>;

template <typename T>
using layout_out_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, __empty>;

template <primitive_type T>
constexpr primitive_t <T> upcast(const T &) { return primitive_t <T> (); }

template <uniform_compatible T>
constexpr T upcast(const T &) { return T(); }

template <synthesizable T>
constexpr T upcast(const T &) { return T(); }

static_assert(std::same_as <layout_in_base <int>, tagged>);

template <typename T, size_t binding>
requires primitive_type <T> || synthesizable <T> || uniform_compatible <T>
struct layout_in : layout_in_base <T> {
	using upcast_t = decltype(upcast(T()));

	std::conditional_t <synthesizable <T> || uniform_compatible <T>,
		cache_index_t, uint8_t> whole_ref;

	layout_in() = default;

	layout_in()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = synthesize_type_fields <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_in(const Args &... args)
	requires uniform_compatible <T> : T(args...), whole_ref(cache_index_t::null()) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		atom::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		whole_ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			atom::Load load;
			load.src = whole_ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}
	}

	cache_index_t synthesize() const
	requires primitive_type <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		atom::Global global;
		global.type = synthesize_type_fields <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_in;

		atom::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit_main(load));
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

	std::conditional_t <synthesizable <T> || uniform_compatible <T>,
		cache_index_t, uint8_t> whole_ref;

	layout_out()
	requires primitive_type <T> = default;

	layout_out()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = synthesize_type_fields <T> ().id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	layout_out(const Args &... args)
	requires uniform_compatible <T> : T(args...), whole_ref(cache_index_t::null()) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		atom::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = binding;
		global.qualifier = atom::Global::layout_out;

		whole_ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			atom::Load load;
			load.src = whole_ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}
	}

	layout_out &operator=(const T &t)
	requires primitive_type <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = synthesize_type_fields <T> ().id;
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
		global.type = synthesize_type_fields <T> ().id;
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
