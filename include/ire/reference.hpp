#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// Buffer reference aggregate wrapper
template <builtin T>
struct buffer_reference_wrapper {
	T value;

	auto layout() {
		return layout_from("Wrapped", verbatim_field(value));
	}
};

// TODO: scalar parameter?
template <generic T>
struct buffer_reference : tagged {
	// TODO: name hint for this...
	buffer_reference() {
		auto &em = Emitter::active;

		thunder::Index type;

		if constexpr (builtin <T>) {
			using WT = buffer_reference_wrapper <T>;

			auto v = WT();
			type = type_info_generator <WT> (v)
				.synthesize()
				.concrete();
		} else {
			auto v = T();
			type = type_info_generator <T> (v)
				.synthesize()
				.concrete();
		}

		auto qual = em.emit_qualifier(type, type_index <T> (), thunder::buffer_reference);

		fmt::println("GENERATED QUAL:");
		em.dump();

		this->ref = qual;
	}

	T operator()(const native_t <uint64_t> &x) {
		auto &em = Emitter::active;

		auto list = em.emit_list(x.synthesize().id);
		auto value = em.emit_construct(this->ref.id, list, thunder::normal);

		if constexpr (builtin <T>) {
			// For builtins, an element is needed within the buffer
			auto result = buffer_reference_wrapper <T> ();
			result.layout().link(value);
			return result.value;
		} else {
			// For aggregates, the fields are expanding within
			fmt::println("REFERENCE WITH AGGREGATE");
			em.dump();
			auto result = T();
			result.layout().link(value);
			return result;
		}
	}
};

} // namespace jvl::ire