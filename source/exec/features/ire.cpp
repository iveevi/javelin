// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include "ire/type_system.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

auto lambda = [](inout <u32> x)
{
	x = x + 1;
};

auto ftn = procedure <void> ("rand") << lambda;

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	fmt::println("{}", link(ftn).generate_glsl());
}
