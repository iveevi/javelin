#pragma once

#include "vector.hpp"

namespace jvl::ire {

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

template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct sampler {
	const size_t binding;

	sampler(size_t binding_ = 0) : binding(binding_) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <vec <T, 4>> ().id;
		thunder::index_t sampler = em.emit_qualifier(type, binding, sampler_qualifiers <T> ::table[D]);
		thunder::index_t value = em.emit_construct(sampler, -1, true);
		return cache_index_t::from(value);
	}
};

using isampler1D = sampler <int32_t, 1>;
using isampler2D = sampler <int32_t, 2>;
using isampler3D = sampler <int32_t, 3>;

using usampler1D = sampler <uint32_t, 1>;
using usampler2D = sampler <uint32_t, 2>;
using usampler3D = sampler <uint32_t, 3>;

using sampler1D = sampler <float, 1>;
using sampler2D = sampler <float, 2>;
using sampler3D = sampler <float, 3>;

// TODO: vec <T, 1> -> T
template <native T, size_t D>
vec <T, 4> texture(const sampler <T, D> &handle, const vec <float, D> &loc)
{
	return platform_intrinsic_from_args <vec <T, 4>> (thunder::glsl_texture, handle, loc);
}

} // namespace jvl::ire