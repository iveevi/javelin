// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include "ire/callable.hpp"
#include "thunder/ad.hpp"
#include "thunder/opt.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>

using namespace jvl;
using namespace jvl::ire;
	
MODULE(ire-features);

auto ftn = callable_info() >> [](f32 x, f32 y) {
	return x * y - x;
};

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	fmt::println("Differentiated:");
	thunder::Buffer buffer;
	thunder::ad_fwd_transform(buffer, ftn);
	buffer.dump();	
	
	thunder::opt_transform(buffer);
	buffer.dump();
}