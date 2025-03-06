// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <queue>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <common/io.hpp>

#include <ire.hpp>

#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: get line numbers for each invocation if possible?
// using source location if available

Procedure pcg3d = procedure("pcg3d") << [](uvec3 v) -> uvec3
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
};

Procedure random3 = procedure("random3") << [](inout <vec3> seed) -> vec3
{
	seed = uintBitsToFloat((pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	return seed;
};

Procedure spherical = procedure("spherical") << [](f32 theta, f32 phi) -> vec3
{
	return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
};

Procedure randomH2 = procedure("randomH2") << [](inout <vec3> seed) -> vec3
{
	vec3 eta = random3(seed);
	f32 theta = acos(eta.x);
	f32 phi = float(2 * M_PI) * eta.y;
	return spherical(theta, phi);
};

namespace jvl::thunder {

// TODO: swizzle elision

} // namespace jvl::thunder

// TODO: optimizer class...?
// optimizer(flags...)(buffer);
// TODO: promote atoms to marked...

struct Payload {
	u32 pindex;
	u32 resolution;

	auto layout() {
		return layout_from("Payload",
			verbatim_field(pindex),
			verbatim_field(resolution));
	}
};

template <floating_arithmetic T>
auto leaky_relu(const T &v)
{
	return max(v, 0.01 * v);
}

Procedure <vec3, vec2> eval = procedure("eval") << [](const vec2 &uv)
{
	task_payload <Payload> payload;

	const uint32_t FEATURE_SIZE = 20;
	const uint32_t ENCODING_LEVELS = 8;
	const uint32_t FFIN = FEATURE_SIZE + 3 * 2 * ENCODING_LEVELS;
	const uint32_t MSIZE = std::max(FFIN, 64u);

	// Neural network weights
	isampler1D ctex(0);
	sampler1D vtex(1);
	sampler2D ftex(2);
	sampler1D biases(3);

	sampler2D w0(4);
	sampler2D w1(5);
	sampler2D w2(6);
	sampler2D w3(7);

	array <f32> A(MSIZE);

	shared <array <vec4>> row(MSIZE);

	array <vec4> B(16);

	ivec4 complex = texelFetch(ctex, i32(payload.pindex), 0);

	vec4 v04 = texelFetch(vtex, complex.x, 0);
	vec4 v14 = texelFetch(vtex, complex.y, 0);
	vec4 v24 = texelFetch(vtex, complex.z, 0);
	vec4 v34 = texelFetch(vtex, complex.w, 0);

	// TODO: swizzle methods...
	vec3 v0 = vec3(v04.x, v04.y, v04.z);
	vec3 v1 = vec3(v14.x, v14.y, v14.z);
	vec3 v2 = vec3(v24.x, v24.y, v24.z);
	vec3 v3 = vec3(v34.x, v34.y, v34.z);

	vec3 vertex = mix(mix(v0, v1, uv.y), mix(v3, v2, uv.y), uv.x);

	// TODO: native overload
	{ auto i = $for(range <u32> (0u, FEATURE_SIZE, 1u));
		vec4 fv = texelFetch(ftex, ivec2(i32(i), i32(payload.pindex)), 0);
		A[i] = mix(mix(fv.x, fv.y, uv.y), mix(fv.w, fv.z, uv.y), uv.x);
	$end(); }

	// Positional encoding
	array <f32> powers = std::array <f32, 8> { 1, 2, 4, 8, 16, 32, 64, 128 };

	u32 k = FEATURE_SIZE;
	{ auto i = $for(range <u32> (0u, ENCODING_LEVELS, 1u));
		f32 p = powers[i];
		vec3 sin_v = sin(p * vertex);
		vec3 cos_v = cos(p * vertex);
		
		A[k] = sin_v.x; k += 1;
		A[k] = sin_v.y; k += 1;
		A[k] = sin_v.z; k += 1;

		A[k] = cos_v.x; k += 1;
		A[k] = cos_v.y; k += 1;
		A[k] = cos_v.z; k += 1;
	$end(); }

	// Network evaluation
	u32 tid = gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x;

	// Layer 0
	// TODO: raw, with fmt references...
	{ auto i = $for(range <i32> (0, 16, 1));
		// Load matrix row into shared memory
		$if(tid == 0);
		{
			auto j = $for(range(i32(0), i32(FFIN), i32(1)));
				row[j] = texelFetch(w0, ivec2(i, j), 0);
			$end();
		}
		$end();

		barrier();

		// Evaluate
		vec4 v = texelFetch(biases, i32(i), 0);

		auto j = $for(range(i32(0), i32(FFIN), i32(1)));
			v += A[j] * row[j];
		$end();

		B[i] = leaky_relu(v);
	$end(); }

	// // Layer 1
	// { auto i = $for(range <i32> (0, 16, 1));
	// 	u32 k = u32(i) << 2;

	// 	// Evaluate
	// 	vec4 v = texelFetch(biases, i32(i) + 16, 0);
	// 	{ auto j = $for(range(i32(0), i32(16), i32(1)));
	// 		i32 l = j << 2;
	// 		vec4 v0 = texelFetch(w1, ivec2(i, l + 0), 0);
	// 		vec4 v1 = texelFetch(w1, ivec2(i, l + 1), 0);
	// 		vec4 v2 = texelFetch(w1, ivec2(i, l + 2), 0);
	// 		vec4 v3 = texelFetch(w1, ivec2(i, l + 3), 0);
	// 		vec4 s = B[j];

	// 		v += s.x * v0 + s.y * v1 + s.z * v2 + s.w * v3;
	// 	$end(); }

	// 	vec4 lv = leaky_relu(v);
	// 	A[k + 0] = lv.x;
	// 	A[k + 1] = lv.y;
	// 	A[k + 2] = lv.z;
	// 	A[k + 3] = lv.w;
	// $end(); }

	// // Layer 2
	// { auto i = $for(range <i32> (0, 16, 1));
	// 	// Evaluate
	// 	vec4 v = texelFetch(biases, i32(i) + 32, 0);
	// 	{ auto j = $for(range <i32> (0, 64, 1));
	// 		v += A[j] * texelFetch(w2, ivec2(i, j), 0);
	// 	$end(); }

	// 	vec4 lv = leaky_relu(v);

	// 	// Fuse with the last layer
	// 	vec4 wx = texelFetch(w3, ivec2(i, 0), 0);
	// 	vec4 wy = texelFetch(w3, ivec2(i, 1), 0);
	// 	vec4 wz = texelFetch(w3, ivec2(i, 2), 0);

	// 	vertex.x += dot(wx, lv);
	// 	vertex.y += dot(wy, lv);
	// 	vertex.z += dot(wz, lv);
	// $end(); }

	vec3 bias = texelFetch(biases, 3 << 6, 0).xyz();

	return vertex + bias;
};

int main()
{
	// thunder::optimize(pcg3d);
	// thunder::optimize(random3);
	// thunder::optimize(spherical);
	// thunder::optimize(randomH2);

	// auto unit = link(randomH2);

	// io::display_lines("RANDOM H2", unit.generate_glsl());

	// unit.write_assembly("ire.jvl.asm");

	io::display_lines("EVAL BEFORE", link(eval).generate_glsl());

	thunder::optimize(eval,
		thunder::OptimizationFlags::eDeadCodeElimination
		+ thunder::OptimizationFlags::eDeduplication
		+ thunder::OptimizationFlags::eStoreElision
	);
	
	io::display_lines("EVAL AFTER", link(eval).generate_glsl());
}