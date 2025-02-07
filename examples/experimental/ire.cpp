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

	EmitMeshTasksExt(groups, groups, 1);
};

auto mesh = procedure <void> ("mesh_shader") << []()
{
	task_payload <payload> payload;

	local_size(8, 8);

	mesh_shader_size(64, 98);
	
	push_constant <config> pc;

	layout_out <unsized_array <vec3>> vertices(0);
	layout_out <unsized_array <u32>> pindex(1);
};

int main()
{
	// thunder::opt_transform(task);
	// task.dump();
	// fmt::println("{}", link(task).generate_glsl());
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
}
