#pragma once

#include "native.hpp"
#include "concepts.hpp"

namespace jvl::ire {

// Forward declarartions
template <generic T>
struct array;

template <generic T>
struct array_base {
	using element = void;
};

template <builtin T>
struct array_base <T> : public tagged {
	using element = T;
	using arithmetic_type = array <T>;

	int32_t length = 0;

	static thunder::Index type(int32_t N) {
		auto &em = Emitter::active;

		auto v = T();
		auto underlying = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		return em.emit_qualifier(underlying, N, thunder::arrays);
	}

	array_base(int32_t N) : length(N) {
		auto &em = Emitter::active;
		this->ref = em.emit_construct(type(N), -1, thunder::normal);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(int32_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		auto l = list_from_args(args...);
		this->ref = em.emit_construct(type(N), l, thunder::normal);
	}

	template <size_t N>
	array_base(const std::array <T, N> &args) : length(N) {
		auto &em = Emitter::active;
		auto l = list_from_array(args);
		this->ref = em.emit_construct(type(N), l, thunder::normal);
	}
	
	array_base(const std::vector <T> &args) : length(args.size()) {
		auto &em = Emitter::active;
		auto l = list_from_vector(args);
		this->ref = em.emit_construct(type(args.size()), l, thunder::normal);
	}
};

template <aggregate T>
struct array_base <T> : public tagged {
	using element = T;
	using arithmetic_type = array <T>;

	int32_t length = 0;

	// TODO: put under one array_type thing...
	static thunder::Index type(int32_t N) {
		auto &em = Emitter::active;

		auto v = T();
		auto underlying = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		return em.emit_qualifier(underlying, N, thunder::arrays);
	}

	array_base(int32_t N) : length(N) {
		auto &em = Emitter::active;
		this->ref = em.emit_construct(type(N), -1, thunder::normal);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array_base(int32_t N, const Args &...args) : length(N) {
		auto &em = Emitter::active;
		auto l = list_from_args(args...);
		this->ref = em.emit_construct(type(N), l, thunder::normal);
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
		if (this->length >= 0 && (index < 0 || index >= this->length))
			JVL_WARNING("index (={}) is out of bounds (size={})", index, this->length);

		auto &em = Emitter::active;
		native_t <int32_t> location(index);
		thunder::Index l = location.synthesize().id;
		thunder::Index c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const
	requires (builtin <T> || native <T>) && (!native <U>) {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");
		auto &em = Emitter::active;
		thunder::Index l = index.synthesize().id;
		thunder::Index c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const
	requires aggregate <T> {
		MODULE(array);

		JVL_ASSERT(this->cached(), "arrays must be cached by the time of use");

		auto &em = Emitter::active;
		auto l = index.synthesize().id;
		auto c = em.emit_array_access(this->ref.id, l);
		
		element returned;
		returned.layout().link(c);
		return returned;
	}
};

template <generic T>
struct unsized_array : array <T> {
	unsized_array() : array <T> (-1) {}
};

// Override type generation
template <typename T>
struct type_info_generator <array <T>> {
	int32_t length;

	type_info_generator(const array <T> &A) : length(A.length) {}

	intermediate_type synthesize() const {
		auto &em = Emitter::active;

		auto v = T();
		auto underlying = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		auto qualifier = em.emit_qualifier(underlying, length, thunder::arrays);

		return composite_type(qualifier);
	}
};

template <typename T>
struct type_info_generator <unsized_array <T>> {
	type_info_generator(const unsized_array <T> &) {}

	intermediate_type synthesize() const {
		auto &em = Emitter::active;

		auto v = T();
		auto underlying = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		auto qualifier = em.emit_qualifier(underlying, -1, thunder::arrays);

		return composite_type(qualifier);
	}
};

// TODO: static array type...

} // namespace jvl::ire