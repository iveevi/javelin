#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/solidify.hpp"
#include "profiles/targets.hpp"
#include "thunder/linkage.hpp"
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

struct TrueMaterial {
	aligned_float3 diffuse;
	aligned_float3 specular;
	aligned_float3 emission;
	aligned_float3 ambient;

	float shininess;
	float roughness;
	float has_albedo;
	float has_normal;
};

// GGX microfacet distribution function
auto ftn = callable_info() >> [](Material mat)
{
	return mat;
	// f32 alpha = mat.roughness;
	// f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
	// f32 ret = (alpha * alpha)
	// 	/ (pi <float> * pow(cos(theta), 4)
	// 	* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	// return ret;
};

int main()
{
	// ftn.dump();
	thunder::detail::legalize_for_cc(ftn);
	thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));

	auto compiled = jit(ftn);

	auto m = solid_t <Material> ();

	m.get <0> () = float3(1, 2, 3);
	m.get <1> () = float3(4, 5, 6);
	m.get <2> () = float3(7, 8, 9);
	m.get <3> () = float3(10, 11, 12);
	m.get <4> () = 13;
	m.get <5> () = 14;
	m.get <6> () = 15;
	m.get <7> () = 16;

	fmt::println("Inputs:");
	fmt::println("  > diffuse: {} {} {}", m.get <0> ().x, m.get <0> ().y, m.get <0> ().z);
	fmt::println("  > specular: {} {} {}", m.get <1> ().x, m.get <1> ().y, m.get <1> ().z);
	fmt::println("  > emission: {} {} {}", m.get <2> ().x, m.get <2> ().y, m.get <2> ().z);
	fmt::println("  > ambient: {} {} {}", m.get <3> ().x, m.get <3> ().y, m.get <3> ().z);
	fmt::println("  > shininess: {}", m.get <4> ());
	fmt::println("  > roughness: {}", m.get <5> ());
	fmt::println("  > has_albedo: {}", m.get <6> ());
	fmt::println("  > has_normal: {}", m.get <7> ());

	auto mr = compiled(m);
	
	fmt::println("Outputs:");
	fmt::println("  > diffuse: {} {} {}", mr.get <0> ().x, mr.get <0> ().y, mr.get <0> ().z);
	fmt::println("  > specular: {} {} {}", mr.get <1> ().x, mr.get <1> ().y, mr.get <1> ().z);
	fmt::println("  > emission: {} {} {}", mr.get <2> ().x, mr.get <2> ().y, mr.get <2> ().z);
	fmt::println("  > ambient: {} {} {}", mr.get <3> ().x, mr.get <3> ().y, mr.get <3> ().z);
	fmt::println("  > shininess: {}", mr.get <4> ());
	fmt::println("  > roughness: {}", mr.get <5> ());
	fmt::println("  > has_albedo: {}", mr.get <6> ());
	fmt::println("  > has_normal: {}", mr.get <7> ());

	auto true_compiled = (TrueMaterial (*)(solid_t <Material>)) compiled;

	auto mrt = true_compiled(m);
	
	fmt::println("Outputs (TM):");
	fmt::println("  > diffuse: {} {} {}", mrt.diffuse.x, mrt.diffuse.y, mrt.diffuse.z);
	fmt::println("  > specular: {} {} {}", mrt.specular.x, mrt.specular.y, mrt.specular.z);
	fmt::println("  > emission: {} {} {}", mrt.emission.x, mrt.emission.y, mrt.emission.z);
	fmt::println("  > ambient: {} {} {}", mrt.ambient.x, mrt.ambient.y, mrt.ambient.z);
	fmt::println("  > shininess: {}", mrt.shininess);
	fmt::println("  > roughness: {}", mrt.roughness);
	fmt::println("  > has_albedo: {}", mrt.has_albedo);
	fmt::println("  > has_normal: {}", mrt.has_normal);
}
