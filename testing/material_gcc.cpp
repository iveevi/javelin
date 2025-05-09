#include <gtest/gtest.h>

#include <glm/glm.hpp>

#include <ire.hpp>

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

	auto layout() {
		return layout_from("Material",
			verbatim_field(diffuse),
			verbatim_field(specular),
			verbatim_field(emission),
			verbatim_field(ambient),
			verbatim_field(shininess),
			verbatim_field(roughness),
			verbatim_field(has_albedo),
			verbatim_field(has_normal));
	}
};

struct ReferenceMaterial {
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;
	alignas(16) glm::vec3 emission;
	alignas(16) glm::vec3 ambient;

	float shininess;
	float roughness;

	bool has_albedo;
	bool has_normal;
};

// // TODO: this is probably crashing...
// // GGX microfacet distribution function
// auto ftn = procedure("ggx_ndf") << [](Material mat, vec3 n, vec3 h)
// {
// 	f32 alpha = mat.roughness;
// 	f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
// 	f32 ret = (alpha * alpha)
// 		/ (std::numbers::pi * pow(cos(theta), 4)
// 		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
// 	return ret;
// };

// auto ref = [](ReferenceMaterial mat, aligned_vec3 n, aligned_vec3 h)
// {
// 	float alpha = mat.roughness;
// 	float theta = std::acos(std::clamp(glm::dot(glm::vec3(n), glm::vec3(h)), 0.0f, 0.999f));
// 	float ret = (alpha * alpha)
// 		/ (std::numbers::pi * glm::pow(glm::cos(theta), 4)
// 		* glm::pow(alpha * alpha + glm::tan(theta) * glm::tan(theta), 2.0f));
// 	return ret;
// };

// // TODO: this test is crashing...
// TEST(ire_jit, material)
// {
// 	thunder::legalize_for_cc(ftn);
// 	thunder::optimize(ftn);
// 	auto compiled = jit(ftn);

// 	auto input = solid_t <Material> ();

// 	input.get <0> () = glm::vec3(1, 2, 3);
// 	input.get <1> () = glm::vec3(4, 5, 6);
// 	input.get <2> () = glm::vec3(7, 8, 9);
// 	input.get <3> () = glm::vec3(10, 11, 12);
// 	input.get <4> () = 13;
// 	input.get <5> () = 14;
// 	input.get <6> () = true;
// 	input.get <7> () = true;

// 	auto output = compiled(input, glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));
	
// 	auto reference_input = ReferenceMaterial();

// 	reference_input.diffuse = glm::vec3(1, 2, 3);
// 	reference_input.specular = glm::vec3(4, 5, 6);
// 	reference_input.emission = glm::vec3(7, 8, 9);
// 	reference_input.ambient = glm::vec3(10, 11, 12);
// 	reference_input.shininess = 13;
// 	reference_input.roughness = 14;
// 	reference_input.has_albedo = true;
// 	reference_input.has_normal = true;

// 	auto reference_output = ref(reference_input, glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));

// 	fmt::println("Output    {}", output);
// 	fmt::println("Expected  {}", reference_output);

// 	ASSERT_EQ(output, reference_output);
// }