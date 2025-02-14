#include "shaders.hpp"

const uint32_t WORK_GROUP_SIZE = 8;

Procedure <void> task = procedure <void> ("main") << []()
{
	push_constant <ViewInfo> pc;

	task_payload <Payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = 4;
	EmitMeshTasksEXT(1, 1, 1);
};

template <floating_arithmetic T>
auto leaky_relu(const T &v)
{
	return max(v, 0.01 * v);
}

auto eval = procedure("eval") << [](const vec2 &uv) -> vec3
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

	array <float> A(MSIZE);

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
	{ auto i = loop(range(u32(0u), u32(FEATURE_SIZE), u32(1u)));
		vec4 fv = texelFetch(ftex, ivec2(i32(i), i32(payload.pindex)), 0);
		A[i] = mix(mix(fv.x, fv.y, uv.y), mix(fv.w, fv.z, uv.y), uv.x);
	end(); }

	// Positional encoding
	array <f32> powers = std::array <f32, 8> { 1, 2, 4, 8, 16, 32, 64, 128 };

	// TODO: this is defined outside the loop...
	// u32 k = FEATURE_SIZE;
	u32 k;
	k = FEATURE_SIZE;

	{ auto i = loop(range(u32(0u), u32(ENCODING_LEVELS), u32(1u)));
		f32 p = powers[i];
		vec3 sin_v = sin(p * vertex);
		vec3 cos_v = cos(p * vertex);
		
		A[k] = sin_v.x; k += 1;
		A[k] = sin_v.y; k += 1;
		A[k] = sin_v.z; k += 1;

		A[k] = cos_v.x; k += 1;
		A[k] = cos_v.y; k += 1;
		A[k] = cos_v.z; k += 1;
	end(); }

	// Network evaluation
	u32 tid = gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x;

	// Layer 0
	// TODO: raw, with fmt references...
	{ auto i = loop(range(i32(0), i32(16), i32(1)));
		// // Load matrix row into shared memory
		// cond(tid == 0);
		// {
		// 	auto j = loop(range(i32(0), i32(FFIN), i32(1)));
		// 		row[j] = texelFetch(w0, ivec2(i, j), 0);
		// 	end();
		// }
		// end();

		// barrier();

		// Evaluate
		vec4 v = texelFetch(biases, i32(i), 0);

		{ auto j = loop(range(i32(0), i32(FFIN), i32(1)));
		{
			// v += A[j] * row[j];
			v += A[j] * texelFetch(w0, ivec2(i, j), 0);
		}
		end(); }

		B[i] = leaky_relu(v);
	end(); }

	// Layer 1
	{ auto i = loop(range(i32(0), i32(16), i32(1)));
		u32 k = u32(i) << 2;

		// Evaluate
		vec4 v = texelFetch(biases, i32(i) + 16, 0);
		{ auto j = loop(range(i32(0), i32(16), i32(1)));
			i32 l = j << 2;
			vec4 v0 = texelFetch(w1, ivec2(i, l + 0), 0);
			vec4 v1 = texelFetch(w1, ivec2(i, l + 1), 0);
			vec4 v2 = texelFetch(w1, ivec2(i, l + 2), 0);
			vec4 v3 = texelFetch(w1, ivec2(i, l + 3), 0);
			vec4 s = B[j];

			v += s.x * v0 + s.y * v1 + s.z * v2 + s.w * v3;
		end(); }

		vec4 lv = leaky_relu(v);
		A[k + 0] = lv.x;
		A[k + 1] = lv.y;
		A[k + 2] = lv.z;
		A[k + 3] = lv.w;
	end(); }

	// Layer 2
	{
		auto i = loop(range(i32(0), i32(16), i32(1)));
		{
			// Evaluate
			vec4 v = texelFetch(biases, i32(i) + 32, 0);
			{
				auto j = loop(range(i32(0), i32(64), i32(1)));
				{
					v += A[j] * texelFetch(w2, ivec2(i, j), 0);
				}
				end();
			}

			vec4 lv = leaky_relu(v);

			// Fuse with the last layer
			vec4 wx = texelFetch(w3, ivec2(i, 0), 0);
			vec4 wy = texelFetch(w3, ivec2(i, 1), 0);
			vec4 wz = texelFetch(w3, ivec2(i, 2), 0);

			vertex.x += dot(wx, lv);
			vertex.y += dot(wy, lv);
			vertex.z += dot(wz, lv);
		}
		end();
	}

	vec4 b4 = texelFetch(biases, 3 << 6, 0);
	vec3 bias = vec3(b4.x, b4.y, b4.z);

	return vertex;
};

// TODO: problems with arrays of one-element structs...

Procedure <void> mesh = procedure <void> ("main") << []()
{
	local_size(WORK_GROUP_SIZE, WORK_GROUP_SIZE);

	mesh_shader_size(64, 98);

	push_constant <ViewInfo> pc;

	task_payload <Payload> payload;

	layout_out <unsized_array <vec3>> positions(0);
	layout_out <unsized_array <vec2>> uvs(1);
	
	u32 res = min(payload.resolution, WORK_GROUP_SIZE);

	u32 trix = res - 1;
	u32 triy = res - 1;

	cond(gl_LocalInvocationIndex >= res * res);
		returns();
	end();

	uvec2 local = uvec2(gl_LocalInvocationIndex / res, gl_LocalInvocationIndex % res);

	vec2 uv = vec2(f32(local.x), f32(local.y)) / f32(res - 1);
	// vec2 uv = vec2(local) / float(res - 1);
	vec3 v = 10 * eval(uv);

	SetMeshOutputsEXT(res * res, 2 * trix * triy);

	u32 gli = gl_LocalInvocationIndex;

	uvs[gli] = uv;
	positions[gli] = v;
	gl_MeshVerticesEXT[gli].gl_Position = pc.project(v);

	cond(local.x < trix && local.y < triy);
	{
		u32 tri = 2 * (local.x + trix * local.y);
		gl_PrimitiveTriangleIndicesEXT[tri] = uvec3(gli, gli + 1, gli + res);
		gl_PrimitiveTriangleIndicesEXT[tri + 1] = uvec3(gli + 1, gli + res + 1, gli + res);
	}
	end();
};

Procedure <void> fragment = procedure <void> ("main") << []()
{
	layout_in <vec3> position(0);
	layout_in <vec2> uv(1);

	layout_out <vec4> fragment(0);
	
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));

	vec3 L = normalize(vec3(1, 1, 1));

	f32 lighting = 0.5 * max(dot(N, L), 0.0f) + 0.5f;

	// fragment = vec4(vec3(uv, 1) * lighting, 1);
	fragment = vec4(0.5 * N + 0.5, 1);
};