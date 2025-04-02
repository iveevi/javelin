// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <common/io.hpp>

#include <ire.hpp>
#include <rexec.hpp>

using namespace jvl;
using namespace jvl::ire;
using namespace jvl::rexec;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: get line numbers for each invocation if possible?
// using source location if available

// Type safe options: shaders as functors...

// In a separate module, author a shader module
// which defines an environment of available
// resources and the corresponding methods under that scope...

// Example:
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

template <size_t Offset>
$rexec_callable(Projection, $push_constant(MVP, Offset))
{
	using self = Projection;

	// Forward declaring to define an interface...
	$rexec_subroutine(vec4, apply, vec3 p) {
		$return $constants.project(p);
	};
};

$rexec_vertex(VShader,
	$layout_in(vec3, 0),
	$layout_in(vec3, 1),
	$layout_out(vec3, 0),
	$push_constant(MVP, 0)
)
{
	$rexec_entrypoint(main) {
		// ...such that the REXEC can still be used appropriately
		gl_Position = $use(Projection <0>).apply($lin(0));
	};
};

$rexec_fragment(FShader,
	$layout_in(vec3, 0),
	$layout_out(vec4, 0)
)
{
	$rexec_entrypoint(main) {
		$lout(0) = vec4(0.5 + 0.5 * $lin(0), 1);
	};
};

// auto pipeline = device << (VShader::main | FShader::main);

// TODO: automatic pipeline generation...

// TODO: linking raw shader (string) code as well...
// use as a test for shader toy examples

int main()
{
	{
		auto glsl = link(VShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// VShader::main.display_assembly();
	}
	
	{
		auto glsl = link(FShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// FShader::main.display_assembly();
	}
}
