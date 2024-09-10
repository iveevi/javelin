#pragma once

#include "ire/type_synthesis.hpp"
#include "primitive.hpp"
#include "tagged.hpp"
#include "thunder/enumerations.hpp"
#include "util.hpp"

namespace jvl::ire {

// Vector types
template <primitive_type T, std::size_t N>
struct vec;

template <primitive_type T, std::size_t N>
requires (N >= 1 && N <= 4)
struct swizzle_base : tagged {};

// Other intrinsics
struct __gl_Position_t;

// Swizzle element
template <primitive_type T, typename Up, thunder::SwizzleCode swz>
class swizzle_element {
	Up *upper;

	swizzle_element(Up *upper_) : upper(upper_) {}
public:
	using base_type = primitive_t <T>;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Swizzle swizzle;
		swizzle.src = upper->synthesize().id;
		swizzle.code = swz;

		cache_index_t ci;
		ci = em.emit(swizzle);
		return ci;
	}

	swizzle_element &operator=(const base_type &v) {
		auto &em = Emitter::active;

		thunder::Store store;
		store.src = v.synthesize().id;
		store.dst = synthesize().id;

		em.emit(store);

		return *this;
	}

	operator base_type() const {
		return base_type(synthesize());
	}

	// Unary minus
	base_type operator-() const {
		return -base_type(synthesize());
	}

	// In place arithmetic operators
	swizzle_element &operator+=(const base_type &a) {
		base_type tmp = *this;
		tmp += a;
		return *this;
	}

	swizzle_element &operator-=(const base_type &a) {
		base_type tmp = *this;
		tmp -= a;
		return *this;
	}

	swizzle_element &operator*=(const base_type &a) {
		base_type tmp = *this;
		tmp *= a;
		return *this;
	}

	swizzle_element &operator/=(const base_type &a) {
		base_type tmp = *this;
		tmp /= a;
		return *this;
	}

	// Bit shift operators
	template <integral_primitive_type U>
	swizzle_element &operator>>=(const primitive_t <U> &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp >>= a;
		return *this;
	}
	
	template <integral_primitive_type U, typename Up_, thunder::SwizzleCode swz_>
	swizzle_element &operator>>=(const swizzle_element <U, Up_, swz_> &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp >>= primitive_t <U> (a);
		return *this;
	}
	
	template <integral_primitive_type U>
	swizzle_element &operator<<=(const primitive_t <U> &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp <<= a;
		return *this;
	}
	
	template <integral_primitive_type U, typename Up_, thunder::SwizzleCode swz_>
	swizzle_element &operator<<=(const swizzle_element <U, Up_, swz_> &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp <<= primitive_t <U> (a);
		return *this;
	}
	
	// Bitwise operators
	swizzle_element &operator|=(const base_type &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp |= a;
		return *this;
	}
	
	swizzle_element &operator&=(const base_type &a)
	requires integral_primitive_type <T> {
		base_type tmp = *this;
		tmp &= a;
		return *this;
	}

	template <primitive_type U, size_t N>
	requires (N >= 1 && N <= 4)
	friend class swizzle_base;

	friend struct __gl_Position_t;
};

// Swizzle base specializations
template <primitive_type T>
class swizzle_base <T, 2> : public tagged {
	using self = swizzle_base <T, 2>;

	T initial[2] = { T(), T() };
public:
	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;

	explicit swizzle_base(T x = T(0), T y = T(0))
			: x(this), y(this) {
		initial[0] = x;
		initial[1] = y;
	}

	swizzle_base(cache_index_t cit) : tagged(cit), x(this), y(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 2>> ().id;
		ctor.args = list_from_args(initial[0], initial[1]);

		return (ref = em.emit(ctor));
	}
};

template <primitive_type T>
class swizzle_base <T, 3> : public tagged {
	using self = swizzle_base <T, 3>;

	T initial[3] = { T(), T() };
public:
	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;
	swizzle_element <T, self, thunder::SwizzleCode::z> z;

	explicit swizzle_base(T x = T(0), T y = T(0), T z = T(0))
			: x(this), y(this), z(this) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
	}

	swizzle_base(const primitive_t <T> &x, const primitive_t <T> &y, const primitive_t <T> &z)
			: swizzle_base() {
		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 3>> ().id;
		ctor.args = list_from_args(x, y, z);

		ref = em.emit(ctor);
	}

	swizzle_base(cache_index_t cit) : tagged(cit), x(this), y(this), z(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;
		thunder::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 3>> ().id;
		ctor.args = list_from_args(initial[0], initial[1], initial[2]);

		return (ref = em.emit(ctor));
	}
};

template <primitive_type T>
class swizzle_base <T, 4> : public tagged {
	using self = swizzle_base <T, 4>;

	T initial[4] = { T(), T(), T(), T() };
public:
	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;
	swizzle_element <T, self, thunder::SwizzleCode::z> z;
	swizzle_element <T, self, thunder::SwizzleCode::w> w;

	explicit swizzle_base(T x = T(0), T y = T(0), T z = T(0), T w = T(0))
			: x(this), y(this), z(this), w(this) {
		initial[0] = x;
		initial[1] = y;
		initial[2] = z;
		initial[3] = w;
	}

	swizzle_base(const swizzle_base <T, 3> &v, const primitive_t <T> &x)
			: swizzle_base() {
		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 4>> ().id;
		ctor.args = list_from_args(v, x);

		ref = em.emit(ctor);
	}

	swizzle_base(cache_index_t cit) : tagged(cit), x(this), y(this), z(this), w(this) {}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		thunder::Construct ctor;
		ctor.type = type_field_from_args <vec <T, 4>> ().id;
		ctor.args = list_from_args(initial[0], initial[1], initial[2], initial[3]);

		return (ref = em.emit(ctor));
	}
};

// Final vector type
template <primitive_type T, std::size_t N>
struct vec : swizzle_base <T, N> {
	using swizzle_base <T, N> ::swizzle_base;

	vec &operator=(const vec &other) {
		auto &em = Emitter::active;

		thunder::Store store;
		store.dst = this->synthesize().id;
		store.src = other.synthesize().id;

		em.emit(store);

		return *this;
	}

	// In place arithmetic operators
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
	#define MIXED_OP(sym, code)								\
		friend vec operator sym(const vec &a, const primitive_t <T> &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}										\
		friend vec operator sym(const primitive_t <T> &a, const vec &b) {		\
			return operation_from_args <vec> (thunder::code, a, b);		\
		}

	MIXED_OP(+, addition);
	MIXED_OP(-, subtraction);
	MIXED_OP(*, multiplication);
	MIXED_OP(/, division);

	// Mixed arithmetic operators with C++ primitives
	#define MIXED_PRIMITIVE_OP(sym, code)							\
		friend vec operator sym(const vec &a, const T &b) {				\
			return a sym primitive_t <T> (b);					\
		}										\
		friend vec operator sym(const T &a, vec &b) {					\
			return primitive_t <T> (a) sym b;					\
		}

	MIXED_PRIMITIVE_OP(+, addition);
	MIXED_PRIMITIVE_OP(-, subtraction);
	MIXED_PRIMITIVE_OP(*, multiplication);
	MIXED_PRIMITIVE_OP(/, division);

	// Common intrinsic functions
	friend vec fract(const vec &a) {
		return platform_intrinsic_from_args <vec> (thunder::fract, a);
	}

	friend vec reflect(const vec &a, const vec &b) {
		return platform_intrinsic_from_args <vec> (thunder::reflect, a, b);
	}
};

// Bitwise operators
template <integral_primitive_type T, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_or, a, b);
}

template <integral_primitive_type T, integral_primitive_type U, size_t N>
vec <T, N> operator|(const vec <T, N>  &a, const U &b)
{
	return a | vec <T, N> (b);
}

template <integral_primitive_type T, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const vec <T, N> &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_and, a, b);
}

template <integral_primitive_type T, integral_primitive_type U, size_t N>
vec <T, N> operator&(const vec <T, N>  &a, const U &b)
{
	return a & vec <T, N> (b);
}

// Shifting operators
template <integral_primitive_type T, integral_primitive_type U, size_t N>
vec <T, N> operator>>(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_right, a, b);
}

template <integral_primitive_type T, integral_primitive_type U, size_t N>
vec <T, N> operator<<(const vec <T, N> &a, const U &b)
{
	return operation_from_args <vec <T, N>> (thunder::bit_shift_left, a, b);
}

} // namespace jvl::ire
