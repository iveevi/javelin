// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct RayFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	// unsized_array <vec3> data;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return layout_from("RayFrame",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical));
	}
};

// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

auto ftn = procedure <void> ("main") << []()
{
	// TODO: composite <..., ..., ...> --> composite <write_only, scalar, buffer, unsized_array <vec3>>
	write_only <scalar <buffer <unsized_array <vec3>>>> bf(0);

	// // TODO: ensure no duplicates...
	// auto tmp1 = buffer_reference <vec2> ();
	// auto tmp2 = buffer_reference <RayFrame> ();

	// u64 x = 0;
	// bf[0] = vec3(tmp1(x).y);
	// bf[1] = tmp2(x).horizontal;

	// TODO: avoid synthesizing arrays...
	auto &em = Emitter::active;
	auto value = em.emit_array_access(bf.ref.id, em.emit_primitive(0));

	buffer_reference_wrapper <vec2> X;

	// push_constant <buffer_reference_wrapper <vec2>> x;
	// bf[0] = vec3(x.value.y);

	push_constant <RayFrame> rf;
	bf[0] = rf.horizontal;

	// X.value.x = 10;

	// X.layout().link(value);

	// X.value.x = 11;
};

// TODO: shadertoy example

int main()
{
	ftn.dump();
	dump_lines("EXPERIMENTAL IRE", link(ftn).generate_glsl());
	link(ftn).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
