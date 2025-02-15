#pragma once

#include "../thunder/enumerations.hpp"
#include "native.hpp"
#include "vector.hpp"

namespace jvl::ire {

// Table of corresponding image QualiferKinds for each scalar type
template <native T>
struct image_qualifiers {};

template <>
struct image_qualifiers <int32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::iimage_1d, // Placeholder
		thunder::iimage_1d,
		thunder::iimage_2d,
		thunder::iimage_3d,
	};
};

template <>
struct image_qualifiers <uint32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::uimage_1d, // Placeholder
		thunder::uimage_1d,
		thunder::uimage_2d,
		thunder::uimage_3d,
	};
};

template <>
struct image_qualifiers <float> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::image_1d, // Placeholder
		thunder::image_1d,
		thunder::image_2d,
		thunder::image_3d,
	};
};

// Image objects
template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct image {
	const size_t binding;

	image(size_t binding_ = 0) : binding(binding_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		// TODO: why do we need this?
		thunder::index_t type = type_field_from_args <vec <T, 4>> ().id;
		thunder::index_t sampler = em.emit_qualifier(type, binding, image_qualifiers <T> ::table[D]);
		thunder::index_t value = em.emit_construct(sampler, -1, thunder::transient);
		return cache_index_t::from(value);
	}

	// Image size
	auto size() const
	requires (D == 1) {
		return platform_intrinsic_from_args <native_t <int32_t>> (thunder::glsl_image_size, *this);
	}

	auto size() const
	requires (D != 1) {
		return platform_intrinsic_from_args <vec <int32_t, D>> (thunder::glsl_image_size, *this);
	}

	// Image load
	vec <T, 4> load(const native_t <int32_t> &loc) const
	requires (D == 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_image_load, *this, loc);
	}
	
	vec <T, 4> load(const vec <int32_t, D> &loc) const
	requires (D != 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_image_load, *this, loc);
	}

	// Image store
	void store(const native_t <int32_t> &loc, const vec <T, 4> &data) const
	requires (D == 1) {
		return void_platform_intrinsic_from_args(thunder::glsl_image_store, *this, loc, data);
	}
	
	void store(const vec <int32_t, D> &loc, const vec <T, 4> &data) const
	requires (D != 1) {
		return void_platform_intrinsic_from_args(thunder::glsl_image_store, *this, loc, data);
	}
};

// Accessors functions from GLSL
template <native T, size_t D>
auto imageSize(const image <T, D> &handle)
{
	return handle.size();
}

template <native T, size_t D, generic I>
auto imageLoad(const image <T, D> &handle, const I &loc)
{
	return handle.load(loc);
}

template <native T, size_t D, generic I>
void imageStore(const image <T, D> &handle, const I &loc, const vec <T, 4> &data)
{
	return handle.store(loc, data);
}

} // namespace jvl::ire