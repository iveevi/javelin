// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

struct payload {
	u32 pindex;
	u32 resolution;

	auto layout() const {
		return uniform_layout("payload",
			named_field(pindex),
			named_field(resolution));
	}
};

struct config {
	mat4 model;
	mat4 view;
	mat4 proj;
	u32 resolution;

	auto layout() const {
		return uniform_layout("config",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(resolution));
	}
};

auto task = procedure <void> ("task_shader") << []()
{
	push_constant <config> pc;

	task_payload <payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = pc.resolution;

	u32 groups = (payload.resolution - 1 + 6) / 7;

	EmitMeshTasksEXT(groups, groups, 1);
};

auto eval = procedure("eval") << [](const vec2 &uv) -> vec3
{
	const uint32_t FEATURE_SIZE = 20;
	const uint32_t ENCODING_LEVELS = 8;
	const uint32_t FFIN = FEATURE_SIZE + 3 * 2 * ENCODING_LEVELS;
	const uint32_t MSIZE = FFIN;

	array <float> A(MSIZE);

	array <vec4> B(16);

	return vec3(uv, 1.0f);
};

auto mesh = procedure <void> ("mesh_shader") << []()
{
	const uint32_t WORK_GROUP_SIZE = 8;

	task_payload <payload> payload;

	local_size(WORK_GROUP_SIZE, WORK_GROUP_SIZE);

	mesh_shader_size(64, 98);
	
	push_constant <config> pc;

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

	vec3 v = eval(uv);

	vertices[gl_LocalInvocationIndex] = v;
	pindex[gl_LocalInvocationIndex] = payload.pindex;
};

int main()
{
	thunder::opt_transform(task);
	task.dump();
	fmt::println("{}", link(task).generate_glsl());
	link(task).generate_spirv(vk::ShaderStageFlagBits::eTaskEXT);
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
	// link(mesh).generate_spirv(vk::ShaderStageFlagBits::eMeshEXT);
}
