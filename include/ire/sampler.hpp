#pragma once

#include "ire/native.hpp"
#include "thunder/enumerations.hpp"
#include "vector.hpp"

namespace jvl::ire {

// Table of corresponding sampler QualiferKinds for each scalar type
template <native T>
struct sampler_qualifiers {};

template <>
struct sampler_qualifiers <int32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::isampler_1d, // Placeholder
		thunder::isampler_1d,
		thunder::isampler_2d,
		thunder::isampler_3d,
	};
};

template <>
struct sampler_qualifiers <uint32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::usampler_1d, // Placeholder
		thunder::usampler_1d,
		thunder::usampler_2d,
		thunder::usampler_3d,
	};
};

template <>
struct sampler_qualifiers <float> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::sampler_1d, // Placeholder
		thunder::sampler_1d,
		thunder::sampler_2d,
		thunder::sampler_3d,
	};
};

// Sampler objects
template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct sampler {
	const size_t binding;

	sampler(size_t binding_ = 0) : binding(binding_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <vec <T, 4>> ().id;
		thunder::index_t sampler = em.emit_qualifier(type, binding, sampler_qualifiers <T> ::table[D]);
		thunder::index_t value = em.emit_construct(sampler, -1, thunder::transient);
		return cache_index_t::from(value);
	}

	// Sampling the associated texture (texture(sampler, uv))
	vec <T, 4> sample(const vec <float, D> &loc) const
	requires (D != 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_texture, *this, loc);
	}

	vec <T, 4> sample(const native_t <float> &loc) const
	requires (D == 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_texture, *this, loc);
	}

	// Fetching pixels from the associated image (texelFetch(sampler, pixel, lod))
	vec <T, 4> fetch(const vec <int32_t, D> &loc, const native_t <int32_t> &lod) const
	requires (D != 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_texelFetch, *this, loc, lod);
	}
	
	vec <T, 4> fetch(const native_t <int32_t> &loc, const native_t <int32_t> &lod) const
	requires (D == 1) {
		return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_texelFetch, *this, loc, lod);
	}
};

// Accessors functions from GLSL
template <native T, size_t D, generic U>
vec <T, 4> texture(const sampler <T, D> &handle, const U &loc)
{
	return handle.sample(loc);
}

template <native T, size_t D, generic A, generic B>
vec <T, 4> texelFetch(const sampler <T, D> &handle, const A &loc, const B &lod)
{
	return handle.fetch(loc, lod);
}

} // namespace jvl::ire