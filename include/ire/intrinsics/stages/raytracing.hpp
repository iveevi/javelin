#pragma once

#include "../../concepts.hpp"
#include "../../aliases.hpp"

namespace jvl::ire {

// TODO: move to different header...
template <generic T>
struct static_constant;

template <native T>
struct static_constant <T> {
	using native_type = T;
	using arithmetic_type = native_t <T>;

	T value;

	static_constant(const T &v) : value(v) {}

	cache_index_t synthesize() const {
		return native_t <T> (value).synthesize();
	}

	operator arithmetic_type() const {
		return arithmetic_type(synthesize());
	}
};

// Acceleration structure for raytracing
struct accelerationStructureEXT {
	int32_t binding;

	accelerationStructureEXT(int32_t binding_ = 0) : binding(binding_) {
		synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto qual = em.emit_qualifier(-1, binding, thunder::acceleration_structure);
		return cache_index_t::from(qual);
	}
};

// Ray flags for tracing
const static_constant <uint32_t> gl_RayFlagsNoneEXT = 0U;
const static_constant <uint32_t> gl_RayFlagsOpaqueEXT = 1U;
const static_constant <uint32_t> gl_RayFlagsNoOpaqueEXT = 2U;
const static_constant <uint32_t> gl_RayFlagsTerminateOnFirstHitEXT = 4U;
const static_constant <uint32_t> gl_RayFlagsSkipClosestHitShaderEXT = 8U;
const static_constant <uint32_t> gl_RayFlagsCullBackFacingTrianglesEXT = 16U;
const static_constant <uint32_t> gl_RayFlagsCullFrontFacingTrianglesEXT = 32U;
const static_constant <uint32_t> gl_RayFlagsCullOpaqueEXT = 64U;
const static_constant <uint32_t> gl_RayFlagsCullNoOpaqueEXT = 128U;

// Trace function
inline void traceRayEXT(const accelerationStructureEXT &as,
			const u32 &flags,
			const u32 &mask,
			const u32 &sbt0,
			const u32 &sbt1,
			const u32 &sbt2,
			const vec3 &origin,
			const f32 &tmin,
			const vec3 &ray,
			const f32 &tmax,
			const i32 &pidx)
{
	return void_platform_intrinsic_from_args(thunder::trace_ray,
		as, flags, mask, sbt0, sbt1, sbt2,
		origin, tmin, ray, tmax, pidx);
}

} // namespace jvl::ire