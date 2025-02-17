#pragma once

#include "native.hpp"

namespace jvl::ire {

/////////////////////
// Swizzle element //
/////////////////////

template <native T, typename Up, thunder::SwizzleCode swz>
class swizzle_element {
	Up &upper;
public:
	using native_type = T;
	using arithmetic_type = native_t <T>;

	swizzle_element(Up &upper_) : upper(upper_) {}
	swizzle_element(Up *upper_) : upper(*upper_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		thunder::Swizzle swizzle;
		swizzle.src = upper.synthesize().id;
		swizzle.code = swz;

		cache_index_t ci;
		ci = em.emit(swizzle);
		return ci;
	}

	swizzle_element &operator=(const arithmetic_type &v) {
		thunder::Store store;
		store.src = v.synthesize().id;
		store.dst = synthesize().id;
		Emitter::active.emit(store);
		return *this;
	}

	operator arithmetic_type() const {
		return arithmetic_type(synthesize());
	}

	// Unary minus
	arithmetic_type operator-() const {
		return -arithmetic_type(synthesize());
	}

	// In place arithmetic operators
	swizzle_element &operator+=(const arithmetic_type &a) {
		arithmetic_type tmp = *this;
		tmp += a;
		return *this;
	}

	swizzle_element &operator-=(const arithmetic_type &a) {
		arithmetic_type tmp = *this;
		tmp -= a;
		return *this;
	}

	swizzle_element &operator*=(const arithmetic_type &a) {
		arithmetic_type tmp = *this;
		tmp *= a;
		return *this;
	}

	swizzle_element &operator/=(const arithmetic_type &a) {
		arithmetic_type tmp = *this;
		tmp /= a;
		return *this;
	}

	// Bit shift operators
	template <integral_native U>
	swizzle_element &operator>>=(const native_t <U> &a)
	requires integral_native <T> {
		thunder::Store store;
		store.src = (arithmetic_type(*this) >> a).synthesize().id;
		store.dst = synthesize().id;
		Emitter::active.emit(store);
		return *this;
	}

	template <integral_native U, typename Up_, thunder::SwizzleCode swz_>
	swizzle_element &operator>>=(const swizzle_element <U, Up_, swz_> &a)
	requires integral_native <T> {
		return operator>>=(native_t <U> (a));
	}

	template <integral_native U>
	swizzle_element &operator<<=(const native_t <U> &a)
	requires integral_native <T> {
		thunder::Store store;
		store.src = (arithmetic_type(*this) << a).synthesize().id;
		store.dst = synthesize().id;
		Emitter::active.emit(store);
		return *this;
	}

	template <integral_native U, typename Up_, thunder::SwizzleCode swz_>
	swizzle_element &operator<<=(const swizzle_element <U, Up_, swz_> &a)
	requires integral_native <T> {
		return operator<<=(native_t <U> (a));
	}

	// Bitwise operators
	swizzle_element &operator|=(const arithmetic_type &a)
	requires integral_native <T> {
		arithmetic_type tmp = *this;
		tmp |= a;
		return *this;
	}

	swizzle_element &operator&=(const arithmetic_type &a)
	requires integral_native <T> {
		arithmetic_type tmp = *this;
		tmp &= a;
		return *this;
	}
};

} // namespace jvl::ire