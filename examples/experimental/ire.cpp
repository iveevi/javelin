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

#include <ire.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

// TODO: for ire compute, pass buffer handle...
auto f = procedure <void> ("main") << []()
{
	write_only_buffer <unsized_array <i32>> results(0);

	i32 k = 10;

	auto i = loop(range <i32> (0, 10, 1));
	{
		results[i] = i * i + k;
		k += 1;
	}
	end();
};

int main()
{
	f.dump();
	fmt::println("{}", link(f).generate_glsl());
	
	thunder::opt_transform(f);

	f.dump();
	fmt::println("{}", link(f).generate_glsl());
}
