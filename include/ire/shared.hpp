#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// TODO: should be concrete_generic (vs. weak_generic involving unsized arrays)
template <generic T>
struct shared : T {
	// Unique index for each shared variable
	static thunder::Index counter() {
		static thunder::Index id = 0;
		return id++;
	}

	// Builtin implementation...
	template <typename ... Args>
	shared(const Args &...args)
	requires builtin <T> : T(args...) {
		auto &em = Emitter::active;

		auto type = type_info_generator <T> (*this)
			.synthesize()
			.concrete();

		auto qual = em.emit_qualifier(type, counter(), thunder::shared);
		auto ctor = em.emit_construct(qual, -1, thunder::global);

		this->ref = cache_index_t::from(ctor);
	}

	// Aggregate implementation...
	template <typename ... Args>
	shared(const Args &...args)
	requires aggregate <T> : T(args...) {
		auto &em = Emitter::active;

		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, counter(), thunder::shared);
		auto ctor = em.emit_construct(qual, -1, thunder::global);

		layout.link(ctor);
	}
};

} // namespace jvl::ire