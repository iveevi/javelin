#include <fmt/format.h>

#include "ire/aggregate.hpp"
#include "ire/core.hpp"
#include "ire/solidify.hpp"
#include "ire/uniform_layout.hpp"
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
// TODO: partial evaluation of callables
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

	// TODO: boolean
	f32 has_albedo;
	f32 has_normal;

	auto layout() const {
		return uniform_layout(
			"Material",
			named_field(diffuse),
			named_field(specular),
			named_field(emission),
			named_field(ambient),
			named_field(shininess),
			named_field(roughness),
			named_field(has_albedo),
			named_field(has_normal)
		);
	}
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

namespace jvl {

auto format_as(const float3 &v)
{
	return fmt::format("({}, {}, {})", v.x, v.y, v.z);
}

}

namespace jvl::ire {

auto format_as(const aligned_float3 &v)
{
	return fmt::format("({}, {}, {})", v.x, v.y, v.z);
}

}

int main()
{
	// ftn.dump();
	thunder::detail::legalize_for_cc(ftn);
	thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));

	auto compiled = jit(ftn);

	auto m_diffuse = uniform_field(Material, diffuse);
	auto m_specular = uniform_field(Material, specular);
	auto m_emission = uniform_field(Material, emission);
	auto m_ambient = uniform_field(Material, ambient);
	auto m_shininess = uniform_field(Material, shininess);
	auto m_roughness = uniform_field(Material, roughness);
	auto m_has_albedo = uniform_field(Material, has_albedo);
	auto m_has_normal = uniform_field(Material, has_normal);

	auto input_material = solid_t <Material> ();

	input_material[m_diffuse] = float3(1, 2, 3);
	input_material[m_specular] = float3(4, 5, 6);
	input_material[m_emission] = float3(7, 8, 9);
	input_material[m_ambient] = float3(10, 11, 12);
	input_material[m_shininess] = 13;
	input_material[m_roughness] = 14;
	input_material[m_has_albedo] = 15;
	input_material[m_has_normal] = 16;

	fmt::println("Inputs:");
	fmt::println("  > diffuse: {}", input_material[m_diffuse]);
	fmt::println("  > specular: {}", input_material[m_specular]);
	fmt::println("  > emission: {}", input_material[m_emission]);
	fmt::println("  > ambient: {}", input_material[m_ambient]);
	fmt::println("  > shininess: {}", input_material[m_specular]);
	fmt::println("  > roughness: {}", input_material[m_roughness]);
	fmt::println("  > has_albedo: {}", input_material[m_has_albedo]);
	fmt::println("  > has_normal: {}", input_material[m_has_normal]);

	auto output_material = compiled(input_material);
	
	fmt::println("Outputs:");
	fmt::println("  > diffuse: {}", output_material[m_diffuse]);
	fmt::println("  > specular: {}", output_material[m_specular]);
	fmt::println("  > emission: {}", output_material[m_emission]);
	fmt::println("  > ambient: {}", output_material[m_ambient]);
	fmt::println("  > shininess: {}", input_material[m_specular]);
	fmt::println("  > roughness: {}", input_material[m_roughness]);
	fmt::println("  > has_albedo: {}", input_material[m_has_albedo]);
	fmt::println("  > has_normal: {}", input_material[m_has_normal]);
}