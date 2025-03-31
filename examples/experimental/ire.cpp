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

// Type safe options: shaders as functors...
struct Vertex {
	gl_Position_t gl_Position;

	void returns() {

	}

	virtual void operator()() = 0;
};

template <generic_or_void R>
struct Method {
	// Restrict certain operations for type checking reasons...
	void returns(const R &) {

	}
};

struct Shader : Vertex {
	void operator()() override {

	}
};

// TODO: linking raw shader (string) code as well...
// use as a test for shader toy examples

$subroutine(i32, demo)(i32 samples)
{
	i32 count;
	f32 ratios;

	$for (i, range(0, samples)) {
		$for (j, range(0, samples)) {
			count += i32(i != j);
			ratios += f32(i) / f32(samples);
		};
	};

	$return count;
};

int main()
{
	{
		auto glsl = link(demo).generate_glsl();
		io::display_lines("FTN", glsl);
	}

	Legalizer::stable.storage(demo);
	Optimizer::stable.apply(demo);
	
	{
		auto glsl = link(demo).generate_glsl();
		io::display_lines("FTN POST", glsl);
	}

	// demo.graphviz("ire.dot");

	demo.display_assembly();
}
