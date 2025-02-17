// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization
// TODO: revised type generation system...
// TODO: atomics

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct RayFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	auto layout() {
		return layout_from("RayFrame",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical));
	}
};

struct accelerationStructureEXT {
	int32_t binding;

	accelerationStructureEXT(int32_t binding_ = 0) : binding(binding_) {
		synthesize();
	}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		auto qual = em.emit_qualifier(-1, binding, thunder::acceleration_structure);
		return cache_index_t::from(qual);
	}
};

auto f = procedure <void> ("main") << []()
{
	push_constant <RayFrame> rayframe;

	ray_payload <vec3> payload1(1);
	ray_payload <vec3> payload2(2);

	accelerationStructureEXT tlas(0);

	// // TODO: assignment operators...
	// ray_payload_in <vec3> payload_in(0);

	write_only <image2D> raw(1);
	vec4 color = vec4(pow(payload1, vec3(1.0 / 2.2)), 1.0);
	ivec2 size = imageSize(raw);
	imageStore(raw, ivec2(0, 1), color);

	// TODO: image formats restricted by native type...
	// image2D image(0);
};

void spill(const std::string &contents)
{
	std::vector <std::string> lines;

	std::string s = contents + "\n";

	std::string interim;
	for (auto c : s) {
		if (c == '\n') {
			lines.emplace_back(interim);
			interim.clear();
			continue;
		}

		interim += c;
	}

	for (size_t i = 0; i < lines.size(); i++)
		fmt::println("{:4d}: {}", i + 1, lines[i]);
}

int main()
{
	f.dump();
	spill(link(f).generate_glsl());
	link(f).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
