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

// // Sandbox application
// struct shuffle_pair {
// 	i32 x;
// 	f32 y;

// 	auto layout() const {
// 		return uniform_layout(
// 			"shuffle_pair",
// 			field <"x"> (x),
// 			field <"y"> (y)
// 		);
// 	}
// };

// f32 __ftn(shuffle_pair sp)
// {
// 	return sp.y;
// 	// sp.x = sp.x >> 5;
// 	// sp.y = sp.y << 17;
// 	// sp.x = (sp.x & 0xff) | (sp.y & 0b10101);
// 	// return sp.x & sp.y;
// }

// auto id = callable(__ftn).named("ftn");

// int main()
// {
// 	// auto kernel = id.export_to_kernel();
// 	// auto linkage = kernel.linkage().resolve();
// 	// std::string source = linkage.generate_cplusplus();
// 	// fmt::println("{}", source);

// 	thunder::opt_transform_compact(id);
// 	// TODO: recursive dead code elimination in one pass...
// 	thunder::opt_transform_dead_code_elimination(id);
// 	thunder::opt_transform_dead_code_elimination(id);
// 	thunder::opt_transform_dead_code_elimination(id);
// 	thunder::opt_transform_dead_code_elimination(id);
// 	thunder::opt_transform_dead_code_elimination(id);
// 	thunder::opt_transform_dead_code_elimination(id);
// 	id.dump();

// 	auto compiled = jit(id);

// 	auto parameter = solid_t <shuffle_pair> ();
// 	parameter.get <0> () = -618;
// 	parameter.get <1> () = 1.618;

// 	fmt::println("compiled(parameter) = {}", compiled(parameter));
// }

struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"mvp",
			field <"model"> (model),
			field <"view"> (view),
			field <"proj"> (proj)
		);
	}
};

// TODO: with regular input/output semantics?
void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> color(0);

	push_constant <mvp> mvp;

	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;
	color = position;
}

void fragment()
{
	layout_in <vec3> position(0);
	layout_out <vec4> fragment(0);

	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

int main()
{
	auto kv = kernel_from_args(vertex);
	kv.dump();

	auto lv = kv.linkage().resolve();
	auto sv = lv.generate_glsl("450");

	fmt::println("\n{}", sv);
}