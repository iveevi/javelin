#pragma once

#include "emitter.hpp"
#include "gltype.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

// For the default 'incompatible' types
template <typename T, size_t binding>
struct layout_in {
	static_assert(!std::is_same_v <T, T>,
		"Unsupported type for layout_in");
};

template <gltype_complatible T, size_t binding>
struct layout_in <T, binding> : tagged {
	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_in;

		op::Load load;
		load.src = em.emit(global);

		return (ref = em.emit_main(load));
	}

	operator T() {
		return T(synthesize());
	}
};

template <uniform_compatible T, size_t binding>
struct layout_in <T, binding> : T {
	cache_index_t ref = cache_index_t::null();

	template <typename ... Args>
	layout_in(const Args &... args) : T(args...) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		op::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = binding;
		global.qualifier = op::Global::layout_in;

		ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			op::Load load;
			load.src = ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}
	}
};

// For the default 'incompatible' types
template <typename T, size_t binding>
struct layout_out {
	static_assert(!std::is_same_v <T, T>,
		"Unsupported type for layout_out");
};

template <gltype_complatible T, size_t binding>
struct layout_out <T, binding> : tagged {
	void operator=(const gltype <T> &t) const {
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

	void operator=(const T &t) const {
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
};

template <synthesizable T, size_t binding>
struct layout_out <T, binding> : T {
	void operator=(const T &t) const {
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
