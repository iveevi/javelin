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

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: get line numbers for each invocation if possible?
// using source location if available

// // Type safe options: shaders as functors...
// struct Vertex {
// 	gl_Position_t gl_Position;

// 	void $return() {

// 	}

// 	virtual void operator()() = 0;
// };

// template <generic_or_void R>
// struct Method {
// 	// Restrict certain operations for type checking reasons...
// 	void $return(const R &) {

// 	}
// };

// struct Shader : Vertex {
// 	void operator()() override {

// 	}
// };

namespace jvl::thunder {

// Since AIR is linear we only need a range to specify any scope
struct Scope {
	Index begin;
	Index end;
};

struct Block : Scope {};

} // namespace jvl::thunder

// TODO: linking raw shader (string) code as well...
// use as a test for shader toy examples

$subroutine(uvec3, pcg3d)(uvec3 v)
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

$subroutine(vec3, random3)(inout <vec3> seed)
{
	seed = uintBitsToFloat((pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	return seed;
};

$subroutine(vec3, spherical)(f32 theta, f32 phi)
{
	return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
};

$subroutine(vec3, randomH2)(inout <vec3> seed)
{
	vec3 eta = random3(seed);
	f32 theta = acos(eta.x);
	f32 phi = float(2 * M_PI) * eta.y;
	return spherical(theta, phi);
};

int main()
{
	auto ftn = randomH2;

	{
		auto glsl = link(ftn).generate_glsl();
		io::display_lines("FTN", glsl);
	}

	Legalizer::stable.storage(ftn);
	Optimizer::stable.apply(ftn);
	
	{
		auto glsl = link(ftn).generate_glsl();
		io::display_lines("FTN POST", glsl);
	}
}
