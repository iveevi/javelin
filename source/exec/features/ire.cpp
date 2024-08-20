#include <fmt/format.h>

#include "ire/core.hpp"
#include "profiles/targets.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: pass structs as inout to callables
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

struct lighting {
	vec3 direction;
	vec3 color;
	f32 area;

	vec3 lambert(vec3 n) const {
		return color * dot(direction, n) * area;
	}

	auto layout() const {
		return uniform_layout(direction, color, area);
	}
};

struct surface_hit {
	vec3 p;
	vec3 n;

	auto layout() const {
		return uniform_layout(p, n);
	}
};

auto gamma_correct = callable([](const vec3 &x) {
	return pow(x, f32(1.0f/2.2f));
}).named("gamma_correct");

auto shade = callable([](const lighting &light,
			 const surface_hit &sh)
{
	return gamma_correct(light.lambert(sh.n) * sh.p);
}).named("shade");

void shader()
{
	// G-buffer inputs
	layout_in <vec3, 0> position;
	layout_in <vec3, 1> normal;

	// Lighting condition
	push_constant <lighting> light;

	// Final color
	layout_out <vec3, 0> color;

	// Construct the surface intersection information
	surface_hit sh {
		position,
		normal
	};

	// Calculate the shaded color
	// NOTE: If shade is passed into the shader(...)
	// then it enables the creation of custom shading
	// pipelines with ease!
	color = shade(light, sh);
}

int main()
{
	auto main_kernel = kernel_from_args(shader);
	auto main_kernel_linkage = main_kernel.linkage();
	main_kernel_linkage.resolve();
	main_kernel_linkage.dump();

	fmt::println("\n{}", main_kernel_linkage.synthesize());
}
