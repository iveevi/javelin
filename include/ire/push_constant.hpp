#pragma once

#include "emitter.hpp"
#include "type_synthesis.hpp"
#include "uniform_layout.hpp"
#include "primitive.hpp"
#include "upcast.hpp"

namespace jvl::ire {

template <typename T>
using push_constant_base = std::conditional_t <builtin <T> || aggregate <T>, T, tagged>;

template <generic T>
struct push_constant : push_constant_base <T> {
	using upcast_t = decltype(upcast(T()));

	// TODO: handle operator= disabling
	push_constant()
	requires native <T> = default;

	push_constant()
	requires builtin <T> {
		auto &em = Emitter::active;

		thunder::Qualifier global;
		global.underlying = type_field_from_args <T> ().id;
		global.kind = thunder::push_constant;

		this->ref = em.emit(global);
	}

	template <typename ... Args>
	push_constant(const Args &... args)
	requires aggregate <T> : T(args...) {
		auto &em = Emitter::active;

		auto layout = this->layout().remove_const();

		thunder::Qualifier global;
		global.underlying = type_field_from_args(layout).id;
		global.kind = thunder::push_constant;

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
		global.kind = thunder::push_constant;

		thunder::Load load;
		load.src = em.emit(global);

		return (this->ref = em.emit(load));
	}

	operator upcast_t ()
	requires native <T> {
		return upcast_t(synthesize());
	}
};

} // namespace jvl::ire
