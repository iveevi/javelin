#pragma once

#include "native.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

// Forward declarartions
template <generic T>
struct array;

// TODO: unsized arrays as well with array <T, -1>
template <generic T>
struct array_base {
	using element = void;
};

template <native T>
struct array_base <T> : public tagged {
	using element = native_t <T>;
	using arithmetic_type = array <T>;

	int32_t length = 0;

	array_base(int32_t N) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, thunder::normal);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(size_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, thunder::normal);
	}
};

template <builtin T>
struct array_base <T> : public tagged {
	using element = T;
	using arithmetic_type = array <T>;

	int32_t length = 0;

	array_base(int32_t N) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, thunder::normal);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(int32_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, thunder::normal);
	}

	template <size_t N>
	array_base(const std::array <T, N> &args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_array(args);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, thunder::normal);
	}
	
	array_base(const std::vector <T> &args) : length(args.size()) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_vector(args);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, length, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, thunder::normal);
	}
};

template <aggregate T>
struct array_base <T> : public tagged {
	using element = T;
	using arithmetic_type = array <T>;

	int32_t length = 0;

	array_base(int32_t N) : length(N) {
		auto &em = Emitter::active;
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, thunder::normal);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(int32_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		auto layout = T().layout();
		thunder::index_t underlying = type_field_from_args(layout).id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, thunder::normal);
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

template <generic T>
struct unsized_array : array <T> {
	unsized_array() : array <T> (-1) {}
};

// Using unsized arrays in global qualifiers
template <typename T>
struct type_info_override <unsized_array <T>> : std::true_type {
	static int synthesize() {
		auto &em = Emitter::active;
		thunder::index_t underlying = type_field_from_args <T> ().id;
		thunder::index_t qualifier = em.emit_qualifier(underlying, -1, thunder::arrays);
		return qualifier;
	}
};

} // namespace jvl::ire