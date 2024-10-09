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
#include <profiles/targets.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

int main()
{
	int degrees = 5;

	auto sh_manual = callable("sh_manual")
		<< std::make_tuple(i32(), degrees)
		<< [](i32 x, int32_t constant)
	{
		// TODO: bug, this isnt being stored into...
		i32 sum = 0;
		auto var = loop(range <i32> (0, constant));
			sum = (sum + var) * x;
		end();
		
		return sum;
	};
	
	sh_manual.dump();

	fmt::println("{}", link(sh_manual).generate_glsl());

	auto sh_closure = callable("sh_closure") << [&](i32 x) {
		i32 sum = 0;
		auto var = loop(range <i32> (0, degrees));
			sum = (sum + var) * x;
		end();

		return sum;
	};

	sh_closure.dump();
	
	fmt::println("{}", link(sh_closure).generate_glsl());
}