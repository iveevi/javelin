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

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: get line numbers for each invocation if possible?
// using source location if available

Procedure B = procedure("pcg3d") << [](uvec3 v) -> uvec3
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
};

Procedure A = procedure("random3") << [](inout <vec3> seed) -> vec3
{
	seed = uintBitsToFloat((B(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	return seed;
};

int main()
{
	std::string pcg3d_shader = link(B).generate_glsl();
	std::string random3_shader = link(A).generate_glsl();

	dump_lines("PCG3D", pcg3d_shader);
	dump_lines("RANDOM3", random3_shader);
	
	link(B).generate_glsl();
	link(A).generate_glsl();
}