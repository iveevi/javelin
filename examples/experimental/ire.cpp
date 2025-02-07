// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include "ire/control_flow.hpp"
#include "thunder/enumerations.hpp"
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

auto task = procedure <void> ("task_shader") << []()
{
	// layout_out <i32> pld(0);
	// pld = 9;
	
	task_payload <i32> pld;
	pld = 9;

	// task_payload <int> pld;
	// pld = 9;
	
	// task_payload <uvec2> pld;
	// pld.y = 9;
	
	// task_payload <payload> pld;
	// pld.pindex = 14;
	// pld.resolution = 16;
};

auto mesh = procedure <void> ("mesh_shader") << []()
{
	local_size(8, 8);

	mesh_shader_size(16, 8);
};

int main()
{
	thunder::opt_transform(task);
	task.dump();
	fmt::println("{}", link(task).generate_glsl());
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
}
