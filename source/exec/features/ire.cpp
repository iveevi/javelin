#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "profiles/targets.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: pass structs as inout to callables
// TODO: creating callables from lambdas
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

struct lighting {
	vec3 direction;
	vec3 color;

	vec3 lambert(vec3 n) const {
		return color * dot(direction, n);
	}

	auto layout() const {
		return uniform_layout(direction, color);
	}
};

struct surface_hit {
	vec3 p;
	vec3 n;

	auto layout() const {
		return uniform_layout(p, n);
	}
};

auto shade = callable([](const lighting &light,
			 const surface_hit &sh)
{
	return light.lambert(sh.n) * sh.p;
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
	{
		auto k = shade.export_to_kernel();
		k.dump();
		std::string source = k.synthesize(profiles::opengl_450);
		fmt::print("\nSOURCE:\n{}", source);
	}

	{
		auto k = kernel_from_args(shader);
		k.dump();
		std::string source = k.synthesize(profiles::opengl_450);
		fmt::print("\nSOURCE:\n{}", source);
	}

	auto l = [](f32 x) -> void { returns(pow(x, 2)); };

	auto kl = callable <f32> (l).named("lamdba");
	kl.export_to_kernel().dump();
	fmt::println("\n{}", kl.export_to_kernel().synthesize(profiles::opengl_450));

	struct functor {
		i32 operator()(i32 x) {
			return x;
		}
	};

	auto x = functor();
	auto kop = callable(x).named("functor");
	kop.export_to_kernel().dump();
	fmt::println("\n{}", kop.export_to_kernel().synthesize(profiles::opengl_450));
}
