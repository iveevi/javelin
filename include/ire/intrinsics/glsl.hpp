#pragma once

#include "../../thunder/atom.hpp"
#include "../../thunder/enumerations.hpp"
#include "../array.hpp"
#include "../tagged.hpp"
#include "../util.hpp"
#include "../vector.hpp"
#include "../swizzle_expansion.hpp"

namespace jvl::ire {

// Immutable instrinsic types
template <generic, thunder::QualifierKind>
struct __glsl_intrinsic_variable_t {};

// Implementation for native types
template <native T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <T, code> {
	using arithmetic_type = native_t <T>;

	__glsl_intrinsic_variable_t() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto type = native_t <T> ::type();
		auto intr = em.emit_qualifier(type, -1, code);
		return cache_index_t::from(intr);
	}

	operator arithmetic_type() const {
		return synthesize();
	}
};

// Implementation for vector types
template <native T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <vec <T, 3>, code> {
	using arithmetic_type = vec <T, 3>;

	using self = __glsl_intrinsic_variable_t;

	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;
	swizzle_element <T, self, thunder::SwizzleCode::z> z;

	SWIZZLE_EXPANSION_DIM3()

	__glsl_intrinsic_variable_t() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto type = vec <T, 3> ::type();
		auto intr = em.emit_qualifier(type, -1, code);
		return cache_index_t::from(intr);
	}

	operator arithmetic_type() const {
		return synthesize();
	}

	// Assignment only for non-const intrinsics (assignable)
	// TODO: requires...
	const self &operator=(const vec <T, 3> &other) {
		auto &em = Emitter::active;
		em.emit_store(synthesize().id, other.synthesize().id);
		return *this;
	}
};

template <native T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <vec <T, 4>, code> {
	using arithmetic_type = vec <T, 4>;

	using self = __glsl_intrinsic_variable_t;

	swizzle_element <T, self, thunder::SwizzleCode::x> x;
	swizzle_element <T, self, thunder::SwizzleCode::y> y;
	swizzle_element <T, self, thunder::SwizzleCode::z> z;
	swizzle_element <T, self, thunder::SwizzleCode::w> w;

	__glsl_intrinsic_variable_t() = default;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto type = vec <T, 4> ::type();
		auto intr = em.emit_qualifier(type, -1, code);
		return cache_index_t::from(intr);
	}

	operator arithmetic_type() const {
		return synthesize();
	}

	// Assignment only for non-const intrinsics (assignable)
	// TODO: requires...
	const self &operator=(const vec <T, 4> &other) {
		auto &em = Emitter::active;
		em.emit_store(synthesize().id, other.synthesize().id);
		return *this;
	}
};

// Implementation for array types
template <generic T, thunder::QualifierKind code>
struct __glsl_intrinsic_variable_t <unsized_array <T>, code> {
	using element = T;
	
	cache_index_t synthesize() const {
		auto &em = Emitter::active;

		auto v = unsized_array <T> ();
		auto array = type_info_generator <decltype(v)> (v)
			.synthesize()
			.concrete();

		auto intr = em.emit_qualifier(array, -1, code);
		return cache_index_t::from(intr);
	}
	
	template <integral_native U>
	element operator[](const U &index) const {
		auto &em = Emitter::active;
		native_t <int32_t> location(index);
		thunder::Index l = location.synthesize().id;
		thunder::Index c = em.emit_array_access(synthesize().id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const {
		auto &em = Emitter::active;
		thunder::Index l = index.synthesize().id;
		thunder::Index c = em.emit_array_access(synthesize().id, l);
		return element(cache_index_t::from(c));
	}
};

// Shorthands
template <thunder::QualifierKind code>
using __glsl_int = __glsl_intrinsic_variable_t <int32_t, code>;

template <thunder::QualifierKind code>
using __glsl_uint = __glsl_intrinsic_variable_t <uint32_t, code>;

template <thunder::QualifierKind code>
using __glsl_float = __glsl_intrinsic_variable_t <float, code>;

template <thunder::QualifierKind code>
using __glsl_vec4 = __glsl_intrinsic_variable_t <vec <float, 4>, code>;

template <thunder::QualifierKind code>
using __glsl_uvec3 = __glsl_intrinsic_variable_t <vec <uint32_t, 3>, code>;

template <generic T, thunder::QualifierKind code>
using __glsl_array = __glsl_intrinsic_variable_t <unsized_array <T>, code>;

////////////////////////////////////////////////
// GLSL shader intrinsic variable definitions //
////////////////////////////////////////////////

using gl_FragCoord_t		= __glsl_vec4  <thunder::glsl_FragCoord>;
using gl_FragDept_t		= __glsl_float <thunder::glsl_FragDepth>;
using gl_InstanceID_t		= __glsl_int   <thunder::glsl_InstanceID>;
using gl_InstanceIndex_t	= __glsl_int   <thunder::glsl_InstanceIndex>;
using gl_VertexID_t		= __glsl_int   <thunder::glsl_VertexID>;
using gl_VertexIndex_t		= __glsl_int   <thunder::glsl_VertexIndex>;
using gl_GlobalInvocationID_t	= __glsl_uvec3 <thunder::glsl_GlobalInvocationID>;
using gl_LocalInvocationID_t	= __glsl_uvec3 <thunder::glsl_LocalInvocationID>;
using gl_LocalInvocationIndex_t	= __glsl_uint  <thunder::glsl_LocalInvocationIndex>;
using gl_WorkGroupID_t		= __glsl_uvec3 <thunder::glsl_WorkGroupID>;
using gl_WorkGroupSize_t	= __glsl_uvec3 <thunder::glsl_WorkGroupSize>;
using gl_SubgroupInvocationID_t	= __glsl_uint <thunder::glsl_SubgroupInvocationID>;
using gl_LaunchIDEXT_t		= __glsl_uvec3 <thunder::glsl_LaunchIDEXT>;
using gl_LaunchSizeEXT_t	= __glsl_uvec3 <thunder::glsl_LaunchSizeEXT>;

static const gl_FragCoord_t		gl_FragCoord;
static const gl_FragDept_t		gl_FragDepth;
static const gl_InstanceID_t		gl_InstanceID;
static const gl_InstanceIndex_t		gl_InstanceIndex;
static const gl_VertexID_t		gl_VertexID;
static const gl_VertexIndex_t		gl_VertexIndex;
static const gl_GlobalInvocationID_t	gl_GlobalInvocationID;
static const gl_LocalInvocationID_t	gl_LocalInvocationID;
static const gl_LocalInvocationIndex_t	gl_LocalInvocationIndex;
static const gl_WorkGroupID_t		gl_WorkGroupID;
static const gl_WorkGroupSize_t		gl_WorkGroupSize;
static const gl_SubgroupInvocationID_t	gl_SubgroupInvocationID;
static const gl_LaunchIDEXT_t		gl_LaunchIDEXT;
static const gl_LaunchSizeEXT_t		gl_LaunchSizeEXT;

// Mutable intrinsics
using gl_Position_t = __glsl_vec4 <thunder::glsl_Position>;

static gl_Position_t	gl_Position;

/////////////////////////////////////
// GLSL shader intrinsic functions //
/////////////////////////////////////

// Operation instrinsics
template <floating_arithmetic A, floating_arithmetic B>
requires (std::same_as <arithmetic_base <A>, arithmetic_base <B>>)
auto mod(const A &x, const B &y)
{
	using result = native_t <typename arithmetic_base <A> ::native_type>;
	return platform_intrinsic_from_args <result> (thunder::mod, underlying(x), underlying(y));
}

template <floating_native A, floating_arithmetic B, size_t N>
requires (std::same_as <A, typename arithmetic_base <B> ::native_type>)
auto mod(const vec <A, N> &x, const B &y)
{
	return platform_intrinsic_from_args <vec <A, N>> (thunder::mod, underlying(x), underlying(y));
}

// Derivative intrinsics
template <native T, size_t N>
vec <T, N> dFdx(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdx, v);
}

template <native T, size_t N>
vec <T, N> dFdy(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdy, v);
}

template <native T, size_t N>
vec <T, N> dFdxFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdxFine, v);
}

template <native T, size_t N>
vec <T, N> dFdyFine(const vec <T, N> &v)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_dFdyFine, v);
}

// Casting intrinsics for scalar types
inline native_t <float> intBitsToFloat(const native_t <int32_t> &v)
{
	return platform_intrinsic_from_args <native_t <float>> (thunder::glsl_intBitsToFloat, v);
}

inline native_t <float> uintBitsToFloat(const native_t <uint32_t> &v)
{
	return platform_intrinsic_from_args <native_t <float>> (thunder::glsl_uintBitsToFloat, v);
}

// Casting intrinsics for vector types
inline native_t <int32_t> floatBitsToInt(const native_t <float> &v)
{
	return platform_intrinsic_from_args <native_t <int32_t>> (thunder::glsl_floatBitsToInt, v);
}

inline native_t <uint32_t> floatBitsToUint(const native_t <float> &v)
{
	return platform_intrinsic_from_args <native_t <uint32_t>> (thunder::glsl_floatBitsToUint, v);
}

template <size_t N>
vec <int32_t, N> floatBitsToInt(const vec <float, N> &v)
{
	return platform_intrinsic_from_args <vec <int32_t, N>> (thunder::glsl_floatBitsToInt, v);
}

template <size_t N>
vec <uint32_t, N> floatBitsToUint(const vec <float, N> &v)
{
	return platform_intrinsic_from_args <vec <uint32_t, N>> (thunder::glsl_floatBitsToUint, v);
}

template <size_t N>
vec <float, N> intBitsToFloat(const vec <int32_t, N> &v)
{
	return platform_intrinsic_from_args <vec <float, N>> (thunder::glsl_intBitsToFloat, v);
}

template <size_t N>
vec <float, N> uintBitsToFloat(const vec <uint32_t, N> &v)
{
	return platform_intrinsic_from_args <vec <float, N>> (thunder::glsl_uintBitsToFloat, v);
}

// Mesh shader intrinsic functions
inline void EmitMeshTasksEXT(const native_t <uint32_t> &x, const native_t <uint32_t> &y, const native_t <uint32_t> &z)
{
	return void_platform_intrinsic_from_args(thunder::emit_mesh_tasks, x, y, z);
}

inline void SetMeshOutputsEXT(const native_t <uint32_t> &x, const native_t <uint32_t> &y)
{
	return void_platform_intrinsic_from_args(thunder::set_mesh_outputs, x, y);
}

// Subgroup intrinsic functions
template <native T, size_t N>
vec <T, N> subgroupShuffle(const vec <T, N> &v, const native_t <uint32_t> &id)
{
	return platform_intrinsic_from_args <vec <T, N>> (thunder::glsl_subgroupShuffle, v, id);
}

// Synchronization functions
inline void barrier()
{
	return void_platform_intrinsic_from_args(thunder::glsl_barrier);
}

} // namespace jvl::ire
