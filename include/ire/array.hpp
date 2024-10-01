#pragma once

#include "ire/native.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

// TODO: unsized arrays as well with array <T, -1>
template <generic T>
struct array_base {
	using element = void;
};

template <native T>
struct array_base <T> : public tagged {
	using element = native_t <T>;

	uint32_t length = 0;

	array_base(size_t N) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(size_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <builtin T>
struct array_base <T> : public tagged {
	using element = T;

	uint32_t length = 0;

	array_base(size_t N) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(size_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}

	template <size_t N>
	array_base(const std::array <T, N> &args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_array(args);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
	
	array_base(const std::vector <T> &args) : length(args.size()) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_vector(args);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, length, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <aggregate T>
struct array_base <T> : public tagged {
	using element = T;

	uint32_t length = 0;

	array_base(size_t N) : length(N) {
		auto &em = Emitter::active;
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(size_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <generic T>
struct array : public array_base <T> {
	using array_base <T> ::array_base;
	using element = typename array_base <T> ::element;

	template <integral_native U>
	element operator[](const U &index) const {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");
		if (index < 0 || index >= this->length)
			JVL_WARNING("index (={}) is out of bounds (size={})", index, this->length);

		auto &em = Emitter::active;
		native_t <int32_t> location(index);
		thunder::index_t l = location.synthesize().id;
		thunder::index_t c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const
	requires (builtin <T> || native <T>) && (!native <U>) {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");
		auto &em = Emitter::active;
		thunder::index_t l = index.synthesize().id;
		thunder::index_t c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const
	requires aggregate <T> {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");
		auto &em = Emitter::active;
		thunder::index_t l = index.synthesize().id;
		thunder::index_t c = em.emit_array_access(this->ref.id, l);
		element returned;
		auto layout = returned.layout().remove_const();
		layout.ref_with(cache_index_t::from(c));
		return returned;
	}
};

} // namespace jvl::ire