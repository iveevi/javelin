#pragma once

#include "emitter.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

template <uniform_compatible T>
struct push_constant : T {
	cache_index_t ref = cache_index_t::null();

	// TODO: handle operator= disabling

	template <typename ... Args>
	push_constant(const Args &... args) : T(args...) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		atom::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = -1;
		global.qualifier = atom::Global::push_constant;

		ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			atom::Load load;
			load.src = ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}

		// TODO: uncached type which clears each time
	}
};

} // namespace jvl::ire
