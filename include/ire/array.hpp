#pragma once

#include "ire/native.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

// TODO: unsized arrays as well with array <T, -1>
template <generic T, thunder::index_t N>
struct array_base {
	using element = void;
};

template <native T, thunder::index_t N>
struct array_base <T, N> : public tagged {
	using element = native_t <T>;

	array_base() {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(const Args &...args) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <builtin T, thunder::index_t N>
struct array_base <T, N> : public tagged {
	using element = T;

	array_base() {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(const Args &...args) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}

	array_base(const std::array <T, N> &args) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_array(args);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <aggregate T, thunder::index_t N>
struct array_base <T, N> : public tagged {
	using element = T;

	array_base() {
		auto &em = Emitter::active;
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(const Args &...args) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}
};

template <generic T, thunder::index_t N>
struct array : public array_base <T, N> {
	using array_base <T, N> ::array_base;
	using element = typename array_base <T, N> ::element;

	template <integral_native U>
	element operator[](const U &index) const {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");
		if (index < 0 || index >= N)
			JVL_WARNING("index (={}) is out of bounds (size={})", index, N);

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

	constexpr int32_t size() const {
		return N;
	}
};

} // namespace jvl::ire