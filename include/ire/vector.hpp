#pragma once

#include "../thunder/enumerations.hpp"
#include "native.hpp"
#include "tagged.hpp"
#include "util.hpp"
#include "swizzle_expansion.hpp"
#include "swizzle_element.hpp"

namespace jvl::ire {

///////////////////////
// Final vector type //
///////////////////////

template <native T, size_t N>
struct vec;

/////////////////////////////////////////
// Basic stencil form for vector types //
/////////////////////////////////////////

template <native T, typename Up, size_t N>
requires (N > 1 && N <= 4)
struct basic_swizzle_base;

template <native T, typename Up>
struct basic_swizzle_base <T, Up, 2> {
	using native_type = T;

	swizzle_element <T, Up, thunder::SwizzleCode::x> x;
	swizzle_element <T, Up, thunder::SwizzleCode::y> y;

	basic_swizzle_base(Up &upper) : x(upper), y(upper) {}
	basic_swizzle_base(Up *upper) : x(upper), y(upper) {}

	SWIZZLE_EXPANSION_DIM2()
};

template <native T, typename Up>
struct basic_swizzle_base <T, Up, 3> {
	using native_type = T;

	swizzle_element <T, Up, thunder::SwizzleCode::x> x;
	swizzle_element <T, Up, thunder::SwizzleCode::y> y;
	swizzle_element <T, Up, thunder::SwizzleCode::z> z;
	
	basic_swizzle_base(Up &upper) : x(upper), y(upper), z(upper) {}
	basic_swizzle_base(Up *upper) : x(upper), y(upper), z(upper) {}

	SWIZZLE_EXPANSION_DIM3()
};

template <native T, typename Up>
struct basic_swizzle_base <T, Up, 4> {
	using native_type = T;

	swizzle_element <T, Up, thunder::SwizzleCode::x> x;
	swizzle_element <T, Up, thunder::SwizzleCode::y> y;
	swizzle_element <T, Up, thunder::SwizzleCode::z> z;
	swizzle_element <T, Up, thunder::SwizzleCode::w> w;
	
	basic_swizzle_base(Up &upper) : x(upper), y(upper), z(upper), w(upper) {}
	basic_swizzle_base(Up *upper) : x(upper), y(upper), z(upper), w(upper) {}

	SWIZZLE_EXPANSION_DIM4()
};

////////////////////////////////////////////////////
// Basic vector types with fleshed out components //
////////////////////////////////////////////////////

template <native T, size_t N>
requires (N > 1 && N <= 4)
class swizzle_base;

template <native T>
class swizzle_base <T, 2> : public tagged, public basic_swizzle_base <T, swizzle_base <T, 2>, 2> {
	using base = basic_swizzle_base <T, swizzle_base <T, 2>, 2>;

	T initial[2] = { T(), T() };
public:
	using native_type = T;

	static constexpr thunder::PrimitiveType primitive() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::ivec2;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::uvec2;
		if constexpr (std::same_as <T, float>)
			return thunder::vec2;
		return thunder::bad;
	}
	
	static constexpr thunder::IntrinsicOperation caster() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::cast_to_ivec2;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::cast_to_uvec2;
		if constexpr (std::same_as <T, float>)
			return thunder::cast_to_vec2;

		return thunder::cast_to_int;
	}

	static thunder::Index type() {
		auto &em = Emitter::active;
		return em.emit_type_information(-1, -1, primitive());
	}

	explicit swizzle_base(T x_ = T(0)) : base(this) {
		initial[0] = x_;
		initial[1] = x_;
		synthesize();
	}

	explicit swizzle_base(T x_, T y_) : base(this) {
		initial[0] = x_;
		initial[1] = y_;
		synthesize();
	}

	explicit swizzle_base(const native_t <T> &x_, const native_t <T> y_) : base(this) {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, y_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		initial[0] = other.initial[0];
		initial[1] = other.initial[1];

		if (other.cached()) {
			auto &em = Emitter::active;
			auto args = em.emit_list(other.ref.id, -1);
			ref = em.emit_construct(type(), args, thunder::normal);
		} else {
			synthesize();
		}
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 2> &other) : base(this) {
		auto &em = Emitter::active;
		synthesize();
		thunder::Index args = em.emit_list(other.synthesize().id);
		thunder::Index cast = em.emit_intrinsic(args, caster());
		em.emit_store(this->ref.id, cast);
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type();
		ctor.args = list_from_args(initial[0], initial[1]);

		ref = em.emit(ctor);

		return ref;
	}
};

template <native T>
class swizzle_base <T, 3> : public tagged, public basic_swizzle_base <T, swizzle_base <T, 3>, 3> {
	using base = basic_swizzle_base <T, swizzle_base <T, 3>, 3>;

	T initial[3] = { T(), T(), T() };
public:
	using native_type = T;

	static constexpr thunder::PrimitiveType primitive() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::ivec3;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::uvec3;
		if constexpr (std::same_as <T, float>)
			return thunder::vec3;
		return thunder::bad;
	}

	static constexpr thunder::IntrinsicOperation caster() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::cast_to_ivec3;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::cast_to_uvec3;
		if constexpr (std::same_as <T, float>)
			return thunder::cast_to_vec3;

		return thunder::cast_to_int;
	}

	static thunder::Index type() {
		auto &em = Emitter::active;
		return em.emit_type_information(-1, -1, primitive());
	}

	explicit swizzle_base(T v_ = T(0)) : base(this) {
		initial[0] = v_;
		initial[1] = v_;
		initial[2] = v_;
		synthesize();
	}

	swizzle_base(T x_ , T y_, T z_) : base(this) {
		initial[0] = x_;
		initial[1] = y_;
		initial[2] = z_;
		synthesize();
	}

	swizzle_base(const native_t <T> &x_) : swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, x_, x_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const native_t <T> &x_,
		     const native_t <T> &y_,
		     const native_t <T> &z_) : swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, y_, z_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}
	
	swizzle_base(const native_t <T> &x_,
		     const swizzle_base <T, 2> &v_) : swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, v_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}
	
	swizzle_base(const swizzle_base <T, 2> &v_,
		     const native_t <T> &z_) : swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(v_, z_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		initial[0] = other.initial[0];
		initial[1] = other.initial[1];
		initial[2] = other.initial[2];

		if (other.cached()) {
			auto &em = Emitter::active;
			auto args = em.emit_list(other.ref.id, -1);
			ref = em.emit_construct(type(), args, thunder::normal);
		} else {
			synthesize();
		}
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 3> &other) : base(this) {
		auto &em = Emitter::active;
		synthesize();
		thunder::Index args = em.emit_list(other.synthesize().id);
		thunder::Index cast = em.emit_intrinsic(args, caster());
		em.emit_store(this->ref.id, cast);
	}
	
	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;
		auto args = list_from_args(initial[0], initial[1], initial[2]);
		ref = em.emit_construct(type(), args, thunder::normal);
		return ref;
	}
};

template <native T>
class swizzle_base <T, 4> : public tagged, public basic_swizzle_base <T, swizzle_base <T, 4>, 4> {
	using base = basic_swizzle_base <T, swizzle_base <T, 4>, 4>;

	T initial[4] = { T(), T(), T(), T() };
public:
	using native_type = T;

	static constexpr thunder::PrimitiveType primitive() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::ivec4;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::uvec4;
		if constexpr (std::same_as <T, float>)
			return thunder::vec4;
		return thunder::bad;
	}

	static constexpr thunder::IntrinsicOperation caster() {
		if constexpr (std::same_as <T, int32_t>)
			return thunder::cast_to_ivec4;
		if constexpr (std::same_as <T, uint32_t>)
			return thunder::cast_to_uvec4;
		if constexpr (std::same_as <T, float>)
			return thunder::cast_to_vec4;

		return thunder::cast_to_int;
	}

	static thunder::Index type() {
		auto &em = Emitter::active;
		return em.emit_type_information(-1, -1, primitive());
	}

	explicit swizzle_base(T x_ = T(0), T y_ = T(0), T z_ = T(0), T w_ = T(0)) : base(this) {
		initial[0] = x_;
		initial[1] = y_;
		initial[2] = z_;
		initial[3] = w_;
		synthesize();
	}

	explicit swizzle_base(const native_t <T> &x_) : swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, x_, x_, x_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	explicit swizzle_base(const native_t <T> &x_,
			      const native_t <T> &y_,
			      const native_t <T> &z_,
			      const native_t <T> &w_)
			: swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(x_, y_, z_, w_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const swizzle_base <T, 3> &v, const native_t <T> &w_)
			: swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(v, w_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const swizzle_base <T, 2> &v, const native_t <T> &z_, const native_t <T> &w_)
			: swizzle_base() {
		auto &em = Emitter::active;
		auto args = list_from_args(v, z_, w_);
		ref = em.emit_construct(type(), args, thunder::normal);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		initial[0] = other.initial[0];
		initial[1] = other.initial[1];
		initial[2] = other.initial[2];
		initial[3] = other.initial[3];

		if (other.cached()) {
			auto &em = Emitter::active;
			auto args = em.emit_list(other.ref.id, -1);
			ref = em.emit_construct(type(), args, thunder::normal);
		} else {
			synthesize();
		}
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 4> &other) : base(this) {
		auto &em = Emitter::active;
		synthesize();
		thunder::Index args = em.emit_list(other.synthesize().id);
		thunder::Index cast = em.emit_intrinsic(args, caster());
		em.emit_store(this->ref.id, cast);
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type();
		ctor.args = list_from_args(initial[0], initial[1], initial[2], initial[3]);

		return (ref = em.emit(ctor));
	}
};

///////////////////////////////////////
// Final vector type; implementation //
///////////////////////////////////////

template <native T, size_t N>
struct vec : swizzle_base <T, N> {
	using swizzle_base <T, N> ::swizzle_base;

	using native_type = T;
	using arithmetic_type = vec <T, N>;

	vec &operator=(const vec &other) {
		auto &em = Emitter::active;

		thunder::Store store;
		store.dst = this->synthesize().id;
		store.src = other.synthesize().id;

		em.emit(store);

		return *this;
	}

	// In place arithmetic operators
	using scalar = native_t <T>;

	vec &operator+=(const vec &a) {
		*this = operation_from_args <vec> (thunder::addition, (*this), a);
		return *this;
	}

	vec &operator-=(const vec &a) {
		*this = operation_from_args <vec> (thunder::subtraction, (*this), a);
		return *this;
	}

	vec &operator*=(const vec &a) {
		*this = operation_from_args <vec> (thunder::multiplication, (*this), a);
		return *this;
	}

	vec &operator*=(const scalar &a) {
		*this = operation_from_args <vec> (thunder::multiplication, (*this), a);
		return *this;
	}

	vec &operator/=(const vec &a) {
		*this = operation_from_args <vec> (thunder::division, (*this), a);
		return *this;
	}

	vec &operator^=(const vec &a) {
		*this = operation_from_args <vec> (thunder::bit_xor, (*this), a);
		return *this;
	}

	// Arithmetic operators
	vec operator-() const {
		return operation_from_args <vec> (thunder::unary_negation, *this);
	}

	friend vec operator+(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::addition, a, b);
	}

	friend vec operator-(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::subtraction, a, b);
	}

	friend vec operator/(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::division, a, b);
	}

	friend vec operator*(const vec &a, const vec &b) {
		return operation_from_args <vec> (thunder::multiplication, a, b);
	}

	// Mixed arithmetic operators
	#define MIXED_OP(sym, code)							\
		friend vec operator sym(const vec &a, const native_t <T> &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}									\
		friend vec operator sym(const native_t <T> &a, const vec &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}

	MIXED_OP(+, addition);
	MIXED_OP(-, subtraction);
	MIXED_OP(*, multiplication);
	MIXED_OP(/, division);

	// Mixed arithmetic operators with C++ primitives
	#define MIXED_PRIMITIVE_OP(sym, code)						\
		friend vec operator sym(const vec &a, const T &b) {			\
			return a sym native_t <T> (b);					\
		}									\
		friend vec operator sym(const T &a, vec &b) {				\
			return native_t <T> (a) sym b;					\
		}

	MIXED_PRIMITIVE_OP(+, addition);
	MIXED_PRIMITIVE_OP(-, subtraction);
	MIXED_PRIMITIVE_OP(*, multiplication);
	MIXED_PRIMITIVE_OP(/, division);

	// Modulus operator
	friend vec operator%(const vec &a, const native_t <T> &b) {
		return operation_from_args <vec> (thunder::modulus, a, b);
	}

	// Common intrinsic functions
	friend vec fract(const vec &a) {
		return platform_intrinsic_from_args <vec> (thunder::fract, a);
	}

	friend vec reflect(const vec &a, const vec &b) {
		return platform_intrinsic_from_args <vec> (thunder::reflect, a, b);
	}
};

// Vector type concept
template <typename T>
struct is_vector_base : std::false_type {};

template <native T, size_t N>
struct is_vector_base <vec <T, N>> : std::true_type {};

template <typename T>
concept vector_arithmetic = is_vector_base <typename T::arithmetic_type> ::value;

// Bitwise operators
template <integral_native T, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_or, a, b);
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const U &b)
{
	return a | vec <T, N> (b);
}

template <integral_native T, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_and, a, b);
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const U &b)
{
	return a & vec <T, N> (b);
}

// Shifting operators
template <integral_native T, integral_native U, size_t N>
vec <T, N> operator>>(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_right, a, native_t <U> (b));
}

template <integral_native T, integral_native U, size_t N>
vec <T, N> operator<<(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_left, a, native_t <U> (b));
}

// Override type generation for vectors
template <native T, size_t D>
struct type_info_generator <vec <T, D>> {
	type_info_generator(const vec <T, D> &) {}

	intermediate_type synthesize() {
		return primitive_type(vec <T, D> ::primitive());
	}
};

} // namespace jvl::ire