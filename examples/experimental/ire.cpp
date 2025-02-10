// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization
// TODO: revised type generation system...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

struct Payload {
	u32 pindex;
	u32 resolution;

	auto layout() const {
		return uniform_layout("payload",
			named_field(pindex),
			named_field(resolution));
	}
};

struct ViewInfo {
	mat4 model;
	mat4 view;
	mat4 proj;
	u32 resolution;

	vec4 project(vec3 v) const {
		return proj * (view * (model * vec4(v, 1)));
	}

	auto layout() const {
		return uniform_layout("config",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(resolution));
	}
};

auto task = procedure <void> ("main") << []()
{
	push_constant <ViewInfo> pc;

	task_payload <Payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = pc.resolution;

	u32 groups = (payload.resolution - 1 + 6) / 7;

	EmitMeshTasksEXT(groups, groups, 1);
};

template <floating_arithmetic T>
auto leaky_relu(const T &v)
{
	return max(v, 0.01 * v);
}

auto eval = procedure("eval") << [](const task_payload <Payload> &payload, const vec2 &uv) -> vec3
{
	const uint32_t FEATURE_SIZE = 20;
	const uint32_t ENCODING_LEVELS = 8;
	const uint32_t FFIN = FEATURE_SIZE + 3 * 2 * ENCODING_LEVELS;
	const uint32_t MSIZE = FFIN;

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

	u32 k = FEATURE_SIZE;
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
	u32 stride = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

	// Layer 0
	{ auto i = loop(range(i32(0), i32(16), i32(1)));
		u32 k = u32(i) << 2;

		// Load matrix row into shared memory
		cond(tid == 0);
		{
			auto j = loop(range(i32(0), i32(FFIN), i32(1)));
				row[j] = texelFetch(w0, ivec2(i, j), 0);
			end();
		}
		end();

		barrier();

		// Evaluate
		vec4 v = texelFetch(biases, i32(i), 0);

		auto j = loop(range(i32(0), i32(FFIN), i32(1)));
			v += A[j] * row[j];
		end();

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
			u32 k = u32(i) << 2;
	
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

	return vertex + bias;
};

// TODO: problems with arrays of one-element structs...

auto mesh = procedure <void> ("main") << []()
{
	const uint32_t WORK_GROUP_SIZE = 8;

	task_payload <Payload> payload;

	local_size(WORK_GROUP_SIZE, WORK_GROUP_SIZE);

	mesh_shader_size(64, 98);
	
	push_constant <ViewInfo> pc;

	layout_out <unsized_array <vec3>> vertices(0);
	layout_out <unsized_array <u32>> pindex(1);

	const uint32_t MAX_QSIZE = WORK_GROUP_SIZE - 1;

	uvec2 offset = uvec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y)
		+ MAX_QSIZE * uvec2(gl_WorkGroupID.x, gl_WorkGroupID.y);

	cond(offset.x > payload.resolution || offset.y > payload.resolution);
		returns();
	end();
	
	uvec2 offset_triangles = MAX_QSIZE * uvec2(gl_WorkGroupID.x, gl_WorkGroupID.y);

	u32 total_qsize = payload.resolution - 1;
	u32 qwidth = min(MAX_QSIZE, total_qsize - offset_triangles.x);
	u32 qheight = min(MAX_QSIZE, total_qsize - offset_triangles.y);

	u32 vwidth = qwidth + 1;
	u32 vheight = qheight + 1;

	SetMeshOutputsEXT(vwidth * vheight, 2 * qwidth * qheight);

	vec2 uv = vec2(f32(offset.x), f32(offset.y)) / f32(payload.resolution - 1);

	vec3 v = eval(payload, uv);

	vertices[gl_LocalInvocationIndex] = v;
	pindex[gl_LocalInvocationIndex] = payload.pindex;
	gl_MeshVerticesEXT[gl_LocalInvocationIndex].gl_Position = pc.project(v);

	u32 gli = gl_LocalInvocationIndex;
	cond(gl_LocalInvocationID.x < qwidth && gl_LocalInvocationID.y < qheight);
	{
		u32 prim = 2 * (gl_LocalInvocationID.x + qwidth * gl_LocalInvocationID.y);

		// Assumes that the subgroup size is 32
		u32 gsi = gl_SubgroupInvocationID;

		f32 sign = 1;
		u32 side = gsi + 1;

		cond(side >= 32 || gl_LocalInvocationID.x >= qwidth);
		{
			side = gsi - 1;
			sign *= -1;
		}
		end();

		u32 vert = gsi + WORK_GROUP_SIZE;

		cond(vert >= 32);
		{
			vert = gsi - WORK_GROUP_SIZE;
			sign *= -1;
		}
		end();

		u32 sidevert = gsi + WORK_GROUP_SIZE + 1;

		cond(sidevert >= 32);
			sidevert = gsi - WORK_GROUP_SIZE + 1;
		end();

		cond(sidevert >= 32);
			sidevert = gsi - WORK_GROUP_SIZE - 1;
		end();

		vec3 sv = subgroupShuffle(v, side);
		vec3 vv = subgroupShuffle(v, vert);
		vec3 svv = subgroupShuffle(v, sidevert);

		f32 d0 = length(v - svv);
		f32 d1 = length(sv - vv);

		cond(d0 > d1);
		{
			gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(gli, gli + 1, gli + WORK_GROUP_SIZE);
			gl_PrimitiveTriangleIndicesEXT[prim + 1] = uvec3(gli + 1, gli + WORK_GROUP_SIZE + 1, gli + WORK_GROUP_SIZE);
		}
		elif();
		{
			gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(gli, gli + 1, gli + WORK_GROUP_SIZE + 1);
			gl_PrimitiveTriangleIndicesEXT[prim + 1] = uvec3(gli, gli + WORK_GROUP_SIZE + 1, gli + WORK_GROUP_SIZE);
		}
		end();
	}
	end();
};

int main()
{
	// thunder::opt_transform(task);
	// task.dump();
	// fmt::println("{}", link(task).generate_glsl());
	// link(task).generate_spirv(vk::ShaderStageFlagBits::eTaskEXT);

	// thunder::opt_transform(eval);
	// eval.dump();
	// fmt::println("{}", link(eval).generate_glsl());
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
	link(mesh).generate_spirv(vk::ShaderStageFlagBits::eMeshEXT);
}
