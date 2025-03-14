// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

#include "common/io.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(mir);

namespace jvl::thunder::mir {

} // namespace jvl::thunder::mir

// TODO: dot generator...
// TODO: glsl function body generator...

auto ftn = procedure("area") << [](vec2 x, vec2 y)
{
	return abs(cross(x, y));
};

int main()
{
	ftn.display_assembly();
	link(ftn).write_assembly("ire.jvl.asm");

	// thunder::optimize_dead_code_elimination(ftn);
	thunder::optimize_deduplicate(ftn);
	ftn.display_assembly();

	auto mir = ftn.lower_to_mir();

	fmt::println("{}", mir);

	mir.graphviz("mir.dot");
}
