// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <queue>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <common/io.hpp>

#include <ire.hpp>

#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

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

Procedure pcg3d = procedure("pcg3d") << [](uvec3 v) -> uvec3
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

Procedure random3 = procedure("random3") << [](inout <vec3> seed) -> vec3
{
	seed = uintBitsToFloat((pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	return seed;
};

Procedure spherical = procedure("spherical") << [](f32 theta, f32 phi) -> vec3
{
	return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
};

Procedure randomH2 = procedure("randomH2") << [](inout <vec3> seed) -> vec3
{
	vec3 eta = random3(seed);
	f32 theta = acos(eta.x);
	f32 phi = float(2 * M_PI) * eta.y;
	return spherical(theta, phi);
};

int main()
{
	thunder::optimize(pcg3d);
	thunder::optimize(random3);
	thunder::optimize(spherical);
	thunder::optimize(randomH2);

	auto unit = link(randomH2);

	io::display_lines("RANDOM H2", unit.generate_glsl());

	unit.write_assembly("ire.jvl.asm");

	random3.graphviz("random3.dot");
}