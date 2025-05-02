#pragma once

#include "swizzle_expansion.hpp"
#include "swizzle_element.hpp"
#include "util.hpp"

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
		construct_override(x_, x_);
	}

	explicit swizzle_base(T x_, T y_) : base(this) {
		construct_override(x_, y_);
	}
	
	swizzle_base(const native_t <T> &x_) : swizzle_base() {
		construct_override(x_, x_);
	}

	explicit swizzle_base(const native_t <T> &x_, const native_t <T> y_) : base(this) {
		construct_override(x_, y_);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		construct_override(other);
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 2> &other) : base(this) {
		auto &em = Emitter::active;
		auto args = em.emit_list(other.synthesize().id);
		this->ref = em.emit_intrinsic(args, caster());
	}

	cache_index_t synthesize() const {
		if (cached())
			return this->ref;

		construct_override(T(), T());

		return this->ref;
	}

	void construct_override(const auto &... args) const {
		this->ref = Emitter::active.emit_construct(type(),
			list_from_args(args...),
			thunder::normal);
	}
};

template <native T>
class swizzle_base <T, 3> : public tagged, public basic_swizzle_base <T, swizzle_base <T, 3>, 3> {
	using base = basic_swizzle_base <T, swizzle_base <T, 3>, 3>;
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
		construct_override(v_, v_, v_);
	}

	swizzle_base(T x_ , T y_, T z_) : base(this) {
		construct_override(x_, y_, z_);
	}

	swizzle_base(const native_t <T> &x_) : swizzle_base() {
		construct_override(x_, x_, x_);
	}

	swizzle_base(const native_t <T> &x_,
		     const native_t <T> &y_,
		     const native_t <T> &z_) : swizzle_base() {
		construct_override(x_, y_, z_);
	}
	
	swizzle_base(const native_t <T> &x_,
		     const swizzle_base <T, 2> &v_) : swizzle_base() {
		construct_override(x_, v_);
	}
	
	swizzle_base(const swizzle_base <T, 2> &v_,
		     const native_t <T> &z_) : swizzle_base() {
		construct_override(v_, z_);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		construct_override(other);
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 3> &other) : base(this) {
		auto &em = Emitter::active;
		auto args = em.emit_list(other.synthesize().id);
		this->ref = em.emit_intrinsic(args, caster());
	}
	
	cache_index_t synthesize() const {
		if (cached())
			return this->ref;

		construct_override(T(), T(), T());

		return this->ref;
	}

	void construct_override(const auto &... args) const {
		this->ref = Emitter::active.emit_construct(type(),
			list_from_args(args...),
			thunder::normal);
	}
};

template <native T>
class swizzle_base <T, 4> : public tagged, public basic_swizzle_base <T, swizzle_base <T, 4>, 4> {
	using base = basic_swizzle_base <T, swizzle_base <T, 4>, 4>;
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
		construct_override(x_, y_, z_, w_);
	}

	explicit swizzle_base(const native_t <T> &x_) : swizzle_base() {
		construct_override(x_, x_, x_, x_);
	}

	explicit swizzle_base(const native_t <T> &x_,
			      const native_t <T> &y_,
			      const native_t <T> &z_,
			      const native_t <T> &w_)
			: swizzle_base() {
		construct_override(x_, y_, z_, w_);
	}

	swizzle_base(const swizzle_base <T, 3> &v, const native_t <T> &w_)
			: swizzle_base() {
		construct_override(v, w_);
	}

	swizzle_base(const swizzle_base <T, 2> &v, const native_t <T> &z_, const native_t <T> &w_)
			: swizzle_base() {
		construct_override(v, z_, w_);
	}

	swizzle_base(const swizzle_base <T, 2> &u, const swizzle_base <T, 2> &v)
			: swizzle_base() {
		construct_override(u, v);
	}

	swizzle_base(const swizzle_base &other) : swizzle_base() {
		construct_override(other);
	}

	swizzle_base(cache_index_t cit) : tagged(cit), base(this) {}

	// Casting conversion
	template <native U>
	swizzle_base(const swizzle_base <U, 4> &other) : base(this) {
		auto &em = Emitter::active;
		auto args = em.emit_list(other.synthesize().id);
		this->ref = em.emit_intrinsic(args, caster());
	}

	cache_index_t synthesize() const {
		if (cached())
			return this->ref;

		construct_override(T(), T(), T(), T());

		return this->ref;
	}

	void construct_override(const auto &... args) const {
		this->ref = Emitter::active.emit_construct(type(),
			list_from_args(args...),
			thunder::normal);
	}
};

} // namespace jvl::ire