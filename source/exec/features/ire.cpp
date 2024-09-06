#include <fmt/format.h>

#include "fmt/base.h"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/scratch.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types
// TODO: static callables from lambda assignment

using namespace jvl;
using namespace jvl::ire;

// Material properties
struct Material {
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	vec3 ambient;

	f32 shininess;
	f32 roughness;

	f32 has_albedo;
	f32 has_normal;

	auto layout() const {
		return uniform_layout(
			"Material",
			field <"diffuse"> (diffuse),
			field <"specular"> (specular),
			field <"emission"> (emission),
			field <"ambient"> (ambient),
			field <"shininess"> (shininess),
			field <"roughness"> (roughness),
			field <"has_albedo"> (has_albedo),
			field <"has_normal"> (has_normal)
		);
	}
};

// Random number generation
uvec3 __pcg3d(uvec3 v)
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
}

auto pcg3d = callable(__pcg3d).named("pcg3d");

uint3 ref(uint3 v)
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
}

int main()
{
	// thunder::opt_transform_compact(pcg3d);
	// thunder::opt_transform_dead_code_elimination(pcg3d);
	// thunder::opt_transform_dead_code_elimination(pcg3d);
	// pcg3d.dump();

	// fmt::println("{}", pcg3d.export_to_kernel().compile(profiles::glsl_450));
	// fmt::println("{}", pcg3d.export_to_kernel().compile(profiles::cplusplus_11));

	thunder::detail::legalize_for_cc(pcg3d);
	thunder::opt_transform_compact(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	pcg3d.dump();

	auto ftn = jit(pcg3d);

	fmt::println("jit(...) = {}", ftn(uint3(10, 2, 3)).x);
	fmt::println("ref(...) = {}", ref(uint3(10, 2, 3)).x);
}
