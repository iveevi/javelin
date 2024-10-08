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

using namespace jvl;
using namespace jvl::ire;

MODULE(ire-features);

struct Structure {
	vec3	x;
	u32	y;
	vec3	z;

	auto layout() const {
		return uniform_layout("S",
			named_field(u32()),
			named_field(vec3()),
			named_field(u32()));
	}
};

template <typename T, size_t F>
auto solid_offset()
{
	T A;
	return ((std::intptr_t) &A.template get <F> () - (std::intptr_t) &A);
}

int main()
{
	// using SolidAlbedo = solid_t <Structure>;
	using SolidAlbedo = solid_builder <0, Structure> ::type;
	
	// static_assert(std::same_as <SolidAlbedo, aggregate_storage <aligned_float3, float3, uint32_t>>);
	// static_assert(std::same_as <SolidAlbedo, aggregate_storage <float3, uint32_t, float3>>);
	// static_assert(std::same_as <SolidAlbedo, aggregate_storage <uint32_t, aligned_float3, float3>>);
	static_assert(std::same_as <SolidAlbedo, aggregate_storage <aligned_uint32_t <16>, float3, uint32_t>>);

	SolidAlbedo a;

	fmt::println("offset of f0 = {}", solid_offset <SolidAlbedo, 0> ());
	fmt::println("offset of f1 = {}", solid_offset <SolidAlbedo, 1> ());
	fmt::println("offset of f1 = {}", solid_offset <SolidAlbedo, 2> ());
}