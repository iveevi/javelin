#pragma once

#include "emitter.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"
#include "primitive.hpp"
#include "upcast.hpp"

namespace jvl::ire {

template <typename T>
using push_constant_base = std::conditional_t <synthesizable <T> || uniform_compatible <T>, T, tagged>;

template <global_qualifier_compatible T>
struct push_constant : push_constant_base <T> {
	using upcast_t = decltype(upcast(T()));

	std::conditional_t <uniform_compatible <T>, cache_index_t, uint8_t> whole_ref;

	// TODO: handle operator= disabling
	push_constant() = default;

	push_constant()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = -1;
		global.qualifier = atom::Global::push_constant;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	push_constant(const Args &... args)
	requires uniform_compatible <T> : T(args...) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		atom::Global global;
		global.type = type_field_from_args(uniform_layout).id;
		global.binding = -1;
		global.qualifier = atom::Global::push_constant;

		whole_ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			atom::Load load;
			load.src = whole_ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}

		// TODO: uncached type which clears each time
	}

	cache_index_t synthesize() const
	requires primitive_type <T> {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		atom::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = -1;
		global.qualifier = atom::Global::push_constant;

		atom::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit_main(load));
	}

	operator upcast_t ()
	requires primitive_type <T> {
		return upcast_t(synthesize());
	}
};

} // namespace jvl::ire
