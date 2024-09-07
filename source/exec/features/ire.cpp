#include <fmt/format.h>

#include "ire/core.hpp"
#include "profiles/targets.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"
#include "logging.hpp"
#include "constants.hpp"

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

// GGX microfacet distribution function
auto ftn = callable_info() >> [](Material mat, vec3 n, vec3 h)
{
	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
	f32 ret = (alpha * alpha)
		/ (pi <float> * pow(cos(theta), 4)
		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	return ret;
};

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));

	jit(ftn);
}
