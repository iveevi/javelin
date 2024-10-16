// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

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

	boolean has_albedo;
	boolean has_normal;

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

static_assert(std::same_as <
	solid_t <Material>,
	aggregate_storage <
		aligned_float3,
		aligned_float3,
		aligned_float3,
		float3,
		float,
		float,
		bool,
		bool
	>
>);

struct ReferenceMaterial {
	aligned_float3 diffuse;
	aligned_float3 specular;
	aligned_float3 emission;
	aligned_float3 ambient;

	float shininess;
	float roughness;

	bool has_albedo;
	bool has_normal;
};

auto ftn = procedure("ggx_ndf") << [](Material mat, vec3 n, vec3 h)
{
	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
	f32 ret = (alpha * alpha)
		/ (pi <float> * pow(cos(theta), 4)
		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	return ret;
};

auto ref = [](ReferenceMaterial mat, aligned_float3 n, aligned_float3 h)
{
	float alpha = mat.roughness;
	float theta = acos(std::clamp(dot(n, h), 0.0f, 0.999f));
	float ret = (alpha * alpha)
		/ (pi <float> * pow(cos(theta), 4)
		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	return ret;
};

int main()
{
	thunder::legalize_for_cc(ftn);
	// thunder::opt_transform(ftn);
	ftn.dump();
	auto compiled = jit(ftn);

	auto m_diffuse = uniform_field(Material, diffuse);
	auto m_specular = uniform_field(Material, specular);
	auto m_emission = uniform_field(Material, emission);
	auto m_ambient = uniform_field(Material, ambient);
	auto m_shininess = uniform_field(Material, shininess);
	auto m_roughness = uniform_field(Material, roughness);
	auto m_has_albedo = uniform_field(Material, has_albedo);
	auto m_has_normal = uniform_field(Material, has_normal);

	auto input = solid_t <Material> ();

	input[m_diffuse] = float3(1, 2, 3);
	input[m_specular] = float3(4, 5, 6);
	input[m_emission] = float3(7, 8, 9);
	input[m_ambient] = float3(10, 11, 12);
	input[m_shininess] = 13;
	input[m_roughness] = 14;
	input[m_has_albedo] = false;
	input[m_has_normal] = true;

	auto output = compiled(input, float3(0, 1, 0), float3(0, 1, 0));
	
	auto reference_input = ReferenceMaterial();

	reference_input.diffuse = float3(1, 2, 3);
	reference_input.specular = float3(4, 5, 6);
	reference_input.emission = float3(7, 8, 9);
	reference_input.ambient = float3(10, 11, 12);
	reference_input.shininess = 13;
	reference_input.roughness = 14;
	reference_input.has_albedo = false;
	reference_input.has_normal = true;

	auto reference_output = ref(reference_input, float3(0, 1, 0), float3(0, 1, 0));
	
	fmt::println("Output    {}", output);
	fmt::println("Expected  {}", reference_output);
}