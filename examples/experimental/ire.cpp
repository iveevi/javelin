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

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct Data {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	unsized_array <vec3> data;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return phantom_layout_from("Data",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical),
			verbatim_field(data));
	}
};

// TODO: type check for soft/concrete types... layout_from only for concrete types

// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

struct Reference {
	u64 triangles;
	u64 vertices;

	auto layout() {
		return layout_from("Reference",
			verbatim_field(triangles),
			verbatim_field(vertices));
	}
};

auto ftn = procedure <void> ("main") << []()
{
	writeonly <scalar <buffer <unsized_array <vec3>>>> bf(0);

	buffer <unsized_array <Reference>> references(1);

	// bf[0] = vec3(10);

	// shared <array <vec4>> arr(16);
	// arr[0] = vec4(0);

	u64 x = references[0].vertices;
	auto br1 = scalar <buffer_reference <unsized_array <vec3>>> (x);
	bf[1] = br1[12];
	
	// auto br2 = buffer_reference <Data> (x);
	// arr[1] = vec4(br2.data[14], 1);
};

// TODO: l-value propagation
// TODO: shadertoy example
// TODO: optimization using graphviz for visualization...

int main()
{
	ftn.dump();
	ftn.graphviz("graph.dot");
	dump_lines("EXPERIMENTAL IRE", link(ftn).generate_glsl());
	link(ftn).generate_spirv(vk::ShaderStageFlagBits::eCompute);
}
