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

	// TODO: handle operator= disabling
	push_constant()
	requires primitive_type <T> = default;

	push_constant()
	requires synthesizable <T> {
		auto &em = Emitter::active;

		thunder::Global global;
		global.type = type_field_from_args <T> ().id;
		global.binding = -1;
		global.qualifier = thunder::GlobalQualifier::push_constant;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	push_constant(const Args &... args)
	requires uniform_compatible <T> : T(args...) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Global global;
		global.type = type_field_from_args(layout).id;
		global.binding = -1;
		global.qualifier = thunder::GlobalQualifier::push_constant;

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
		global.binding = -1;
		global.qualifier = thunder::GlobalQualifier::push_constant;

		thunder::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit_main(load));
	}

	operator upcast_t ()
	requires primitive_type <T> {
		return upcast_t(synthesize());
	}
};

} // namespace jvl::ire
