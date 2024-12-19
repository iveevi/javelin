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

auto tea = procedure("tea") << [](u32 val0, u32 val1)
{
	u32 v0 = val0;
	u32 v1 = val1;
	u32 s0 = 0;

	for (uint32_t i = 0; i < 16; i++) {
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4u) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5u) + 0xc8013ea4);
		v1 += ((v0 << 4u) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5u) + 0x7e95761e);
	}

	return v0;
};

auto lambda = [](inout <u32> x)
{
	return tea(x, 1 - x);
};

auto ftn = procedure <void> ("rand") << lambda;

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	fmt::println("{}", link(ftn).generate_glsl());
}
