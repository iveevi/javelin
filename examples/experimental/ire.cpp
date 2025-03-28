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
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
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

// TODO: macro for this...
#define func(name) Procedure name = procedure(#name) << []

func(ftn)(vec2 uv, i32 samples, vec2 resolution)
{
	i32 count;

	auto it = range <i32> (0, samples, 1);
	
	auto i = $for(it);
	{
		auto j = $for(it);
		{
			count += i32(i != j);
		}
		$end();
	}
	$end();

	$return(count);
};

// TODO: legalize stores through a storage atom...

int main()
{
	auto glsl = link(ftn).generate_glsl();
	io::display_lines("FTN", glsl);
	// ftn.display_assembly();
	ftn.graphviz("ire.dot");

	// TODO: fix deduplication by doing l-value propogation...
	// TODO: fix optimization around blocks...
	thunder::optimize(ftn, thunder::OptimizationFlags::eDeadCodeElimination);

	auto glsl_opt = link(ftn).generate_glsl();
	io::display_lines("FTN OPT", glsl_opt);
	// ftn.display_assembly();
	ftn.graphviz("ire-optimized.dot");
}
