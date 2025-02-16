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

template <native T, size_t D>
struct type_info_override <image <T, D>> : std::true_type {
	thunder::index_t binding = 0;

	type_info_override(const image <T, D> &img) : binding(img.binding) {}

	int synthesize() const {
		return Emitter::active.emit_qualifier(-1, binding, image_qualifiers <T> ::table[D]);
	}
};

// Extra qualifiers
template <builtin T>
struct write_only : T {
	static_assert(false, "write_only is unsupported for given template type");
};

template <native T, size_t D>
struct write_only <image <T, D>> : image <T, D> {
	template <typename ... Args>
	write_only(const Args &... args) : image <T, D> (args...) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		// thunder::index_t nested = type_field_from_args <image <T, D>> ().id;
		thunder::index_t nested = em.emit_qualifier(-1, this->binding, image_qualifiers <T> ::table[D]);
		thunder::index_t qualifier = em.emit_qualifier(nested, -1, thunder::write_only);
		thunder::index_t value = em.emit_construct(qualifier, -1, thunder::transient);
		return cache_index_t::from(value);
	}
};

template <native T, size_t D>
struct is_image_like <write_only <image <T, D>>> : std::true_type {};

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

	imageStore(image, ivec2(0, 0), vec4(0.0));
};

int main()
{
	f.dump();
	fmt::println("{}", link(f).generate_glsl());
	link(f).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
