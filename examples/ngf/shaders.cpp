#include <common/io.hpp>

#include "shaders.hpp"

const uint32_t WORK_GROUP_SIZE = 8;

$entrypoint(task)
{
	push_constant <ViewInfo> pc;

	task_payload <Payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = pc.resolution;
	EmitMeshTasksEXT(1, 1, 1);
};

template <floating_arithmetic T>
auto leaky_relu(const T &v)
{
	return max(v, 0.01 * v);
}

$subroutine(vec3, eval, vec2 uv)
{
	task_payload <Payload> payload;

	const int32_t FEATURE_SIZE = 20;
	const int32_t ENCODING_LEVELS = 8;
	const int32_t FFIN = FEATURE_SIZE + 3 * 2 * ENCODING_LEVELS;
	const int32_t MSIZE = std::max(FFIN, 64);

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

	$for (i, range(0, FEATURE_SIZE)) {
		vec4 fv = texelFetch(ftex, ivec2(i32(i), i32(payload.pindex)), 0);
		A[i] = mix(mix(fv.x, fv.y, uv.y), mix(fv.w, fv.z, uv.y), uv.x);
	};

	// Positional encoding
	array <f32> powers = std::array <f32, 8> {
		 1,  2,  4,   8,
		16, 32, 64, 128,
	};

	u32 k = FEATURE_SIZE;

	$for (i, range(0, ENCODING_LEVELS)) {
		f32 p = powers[i];
		vec3 sin_v = sin(p * vertex);
		vec3 cos_v = cos(p * vertex);
		
		A[k] = sin_v.x; k += 1;
		A[k] = sin_v.y; k += 1;
		A[k] = sin_v.z; k += 1;

		A[k] = cos_v.x; k += 1;
		A[k] = cos_v.y; k += 1;
		A[k] = cos_v.z; k += 1;
	};

	// Network evaluation
	u32 tid = gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x;

	// Layer 0
	$for (i, range(0, 16)) {
		// Load matrix row into shared memory
		$if (tid == 0) {
			$for (j, range(0, FFIN)) {
				row[j] = texelFetch(w0, ivec2(i, j), 0);
			};
		};

		barrier();

		// Evaluate
		vec4 v = texelFetch(biases, i32(i), 0);

		$for (j, range(0, FFIN)) {
			v += A[j] * row[j];
		};

		B[i] = leaky_relu(v);
	};

	// Layer 1
	$for (i, range(0, 16)) {
		u32 k = u32(i) << 2;

		// Evaluate
		vec4 v = texelFetch(biases, i32(i) + 16, 0);
		$for (j, range(0, 16)) {
			i32 l = j << 2;
			vec4 v0 = texelFetch(w1, ivec2(i, l + 0), 0);
			vec4 v1 = texelFetch(w1, ivec2(i, l + 1), 0);
			vec4 v2 = texelFetch(w1, ivec2(i, l + 2), 0);
			vec4 v3 = texelFetch(w1, ivec2(i, l + 3), 0);
			vec4 s = B[j];

			v += s.x * v0 + s.y * v1 + s.z * v2 + s.w * v3;
		};

		vec4 lv = leaky_relu(v);
		A[k + 0] = lv.x;
		A[k + 1] = lv.y;
		A[k + 2] = lv.z;
		A[k + 3] = lv.w;
	};

	// Layer 2
	$for (i, range(0, 16)) {
		// Evaluate
		vec4 v = texelFetch(biases, i32(i) + 32, 0);
		$for (j, range(0, 64)) {
			v += A[j] * texelFetch(w2, ivec2(i, j), 0);
		};

		vec4 lv = leaky_relu(v);

		// Fuse with the last layer
		vec4 wx = texelFetch(w3, ivec2(i, 0), 0);
		vec4 wy = texelFetch(w3, ivec2(i, 1), 0);
		vec4 wz = texelFetch(w3, ivec2(i, 2), 0);

		vertex.x += dot(wx, lv);
		vertex.y += dot(wy, lv);
		vertex.z += dot(wz, lv);
	};

	vec4 b4 = texelFetch(biases, 3 << 6, 0);
	vec3 bias = vec3(b4.x, b4.y, b4.z);

	$return vertex + bias;
};

// TODO: problems with arrays of one-element structs...

$entrypoint(mesh)
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

	$if (gl_LocalInvocationIndex >= res * res) {
		$return $void;
	};

	uvec2 local = uvec2(gl_LocalInvocationIndex / res, gl_LocalInvocationIndex % res);

	vec2 uv = vec2(f32(local.x), f32(local.y)) / f32(res - 1);
	// vec2 uv = vec2(local) / float(res - 1);
	vec3 v = 10 * eval(uv);

	SetMeshOutputsEXT(res * res, 2 * trix * triy);

	u32 gli = gl_LocalInvocationIndex;

	uvs[gli] = uv;
	positions[gli] = v;
	gl_MeshVerticesEXT[gli].gl_Position = pc.project(v);

	$if (local.x < trix && local.y < triy) {
		u32 tri = 2 * (local.x + trix * local.y);
		gl_PrimitiveTriangleIndicesEXT[tri] = uvec3(gli, gli + 1, gli + res);
		gl_PrimitiveTriangleIndicesEXT[tri + 1] = uvec3(gli + 1, gli + res + 1, gli + res);
	};
};

$entrypoint(fragment)
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

// Debugging
void shader_debug()
{
	static const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path() / ".." / "..";
	static const std::filesystem::path local = root / "output" / "ngf";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);

	std::string task_shader = link(task).generate_glsl();
	std::string mesh_shader = link(mesh).generate_glsl();
	std::string eval_shader = link(eval).generate_glsl();
	std::string fragment_shader = link(fragment).generate_glsl();

	io::display_lines("TASK", task_shader);
	io::display_lines("MESH", mesh_shader);
	io::display_lines("EVAL", eval_shader);
	io::display_lines("FRAGMENT", fragment_shader);

	task.graphviz(local / "task.dot");
	mesh.graphviz(local / "mesh.dot");
	eval.graphviz(local / "eval.dot");
	fragment.graphviz(local / "fragment.dot");

	Optimizer::stable.apply(task);
	Optimizer::stable.apply(mesh);
	Optimizer::stable.apply(eval);

	Optimizer::stable.apply(fragment);

	task.graphviz(local / "task-optimize.dot");
	mesh.graphviz(local / "mesh-optimize.dot");
	eval.graphviz(local / "eval-optimize.dot");
	fragment.graphviz(local / "fragment-optimize.dot");

	link(task, mesh, eval, fragment).write_assembly(local / "shaders.jvl.asm");
}