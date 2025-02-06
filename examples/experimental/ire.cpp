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
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

template <generic T>
struct task_payload {};

auto task = procedure <void> ("task_shader") << []()
{
	local_size(8, 8);

	mesh_shader_size(16, 8);
};

auto mesh = procedure <void> ("mesh_shader") << []()
{
	local_size(8, 8);

	mesh_shader_size(16, 8);
};

int main()
{
	thunder::opt_transform(mesh);
	mesh.dump();

	fmt::println("{}", link(mesh).generate_glsl());
}
