#pragma once

#include "emitter.hpp"
#include "gltype.hpp"
#include "ire/tagged.hpp"
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

template <gltype_complatible T>
constexpr gltype <T> upcast(const T &) { return gltype <T> (); }

template <uniform_compatible T>
constexpr T upcast(const T &) { return T(); }

template <synthesizable T>
constexpr T upcast(const T &) { return T(); }

static_assert(std::same_as <layout_in_base <int>, tagged>);

template <typename T, size_t binding>
requires gltype_complatible <T> || synthesizable <T> || uniform_compatible <T>
struct layout_in : layout_in_base <T> {
	using upcast_t = decltype(upcast(T()));

	std::conditional_t <synthesizable <T> || uniform_compatible <T>,
		cache_index_t, uint8_t> whole_ref;

	layout_in() = default;

	template <typename ... Args>
	layout_in(const Args &... args)
	requires uniform_compatible <T> : T(args...), whole_ref(cache_index_t::null()) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		op::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = binding;
		global.qualifier = op::Global::layout_in;

		whole_ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			op::Load load;
			load.src = whole_ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}
	}

	cache_index_t synthesize() const
	requires gltype_complatible <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_in;

		op::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit_main(load));
	}

	operator gltype <T> ()
	requires gltype_complatible <T> {
		return gltype <T> (synthesize());
	}
};

template <typename T, size_t binding>
requires gltype_complatible <T> || synthesizable <T> || uniform_compatible <T>
struct layout_out : layout_out_base <T> {
	using upcast_t = decltype(upcast(T()));

	void operator=(const T &t) const
	requires gltype_complatible <T> {
		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_out;

		op::Store store;
		store.dst = em.emit(global);
		store.src = translate_primitive(t);

		em.emit_main(store);
	}

	void operator=(const upcast_t &t) const {
		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_out;

		op::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit_main(store);
	}
};

}
