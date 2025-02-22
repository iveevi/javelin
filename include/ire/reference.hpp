#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// Buffer reference aggregate wrapper
template <generic T>
struct buffer_reference_wrapper {
	T value;

	auto layout() {
		return layout_from("Reference", verbatim_field(value));
	}
};

// TODO: scalar parameter?
template <generic T>
struct buffer_reference : tagged {
	// TODO: name hint for this...
	buffer_reference() {
		auto &em = Emitter::active;

		auto v = T();
		auto type = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		auto qual = em.emit_qualifier(type, type_index <T> (), thunder::buffer_reference);

		this->ref = qual;
	}

	T operator()(const native_t <uint64_t> &x) {
		auto &em = Emitter::active;

		auto list = em.emit_list(x.synthesize().id);
		auto value = em.emit_construct(this->ref.id, list, thunder::normal);

		auto wrapped = buffer_reference_wrapper <T> ();
		wrapped.layout().link(value);
		return wrapped.value;
	}
};

} // namespace jvl::ire