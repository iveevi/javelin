#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types

using namespace jvl;
using namespace jvl::ire;

// Sandbox application
struct shuffle_pair {
	i32 x;
	i32 y;

	auto layout() const {
		return uniform_layout(
			"shuffle_pair",
			field <"x"> (x),
			field <"y"> (y)
		);
	}
};

i32 __ftn(shuffle_pair sp)
{
	return (sp.x >> 5) ^ (sp.y << 17);

	// sp.x = sp.x >> 5;
	// sp.y = sp.y << 17;
	// sp.x = (sp.x & 0xff) | (sp.y & 0b10101);
	// return sp.x & sp.y;
}

auto id = callable(__ftn).named("ftn");

int main()
{
	// auto kernel = id.export_to_kernel();
	// auto linkage = kernel.linkage().resolve();
	// std::string source = linkage.generate_cplusplus();
	// fmt::println("{}", source);

	thunder::opt_transform_compact(id);
	// // TODO: recursive dead code elimination in one pass...
	thunder::opt_transform_dead_code_elimination(id);
	// thunder::opt_transform_dead_code_elimination(id);
	// thunder::opt_transform_dead_code_elimination(id);
	// thunder::opt_transform_dead_code_elimination(id);
	// thunder::opt_transform_dead_code_elimination(id);
	// thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto compiled = jit(id);

	auto parameter = solid_t <shuffle_pair> ();
	parameter.get <0> () = 0x56;
	parameter.get <1> () = 0xff;

	fmt::println("compiled(parameter) = {:x}", compiled(parameter));
}