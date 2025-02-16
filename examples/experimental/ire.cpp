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

struct RayFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	auto layout() const {
		return uniform_layout("RayFrame",
			named_field(origin),
			named_field(lower_left),
			named_field(horizontal),
			named_field(vertical));
	}
};

struct Nested {
	vec3 x;
	RayFrame frame;

	auto layout() const {
		return uniform_layout("Nested",
			named_field(x),
			named_field(frame));
	}
};

auto f = procedure <void> ("main") << []()
{
	// push_constant <RayFrame> rayframe;

	// ray_payload <vec3> payload1(1);
	// ray_payload <vec3> payload2(2);

	// // TODO: assignment operators...
	// ray_payload_in <vec3> payload_in(0);

	// image2D raw(1);
	// image1D raw2(2);

	// vec4 color = vec4(pow(payload1, vec3(1.0 / 2.2)), 1.0);

	// ivec2 size = imageSize(raw);
	// i32 s2 = imageSize(raw2);

	// imageStore(raw, ivec2(0, 1), color);

	// color = imageLoad(raw2, 12);

	// TODO: image formats restricted by native type...
	// image2D image(0);
	write_only <image2D> image(0);
	read_only <uimage2D> rimage(1);

	imageStore(image, ivec2(0, 0), vec4(0.0));

	uvec4 c = imageLoad(rimage, ivec2(10, 100));
};

int main()
{
	f.dump();
	fmt::println("{}", link(f).generate_glsl());
	link(f).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
