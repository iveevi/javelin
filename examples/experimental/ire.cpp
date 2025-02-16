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

/////////////////////////////////////////
// Type and qualifier generator system //
/////////////////////////////////////////

struct primitive_type {
	thunder::PrimitiveType primitive;
};

struct struct_type {
	// TODO: capitalize index...
	thunder::index_t idx;
};

using intermediate_type = bestd::variant <primitive_type, struct_type>;

// General form is explicitly a struct with fields of type Args...
template <generic ... Args>
requires (sizeof...(Args) > 0)
struct type_info_generator {
	static constexpr size_t count = sizeof...(Args);

	using tuple_type = std::tuple <Args...>;

	template <size_t I>
	using tuple_element = std::tuple_element_t <I, tuple_type>;

	tuple_type args;

	type_info_generator(const Args &... args_) : args(args_...) {}

	template <size_t I>
	intermediate_type partial() {
		auto &em = Emitter::active;

		intermediate_type current = type_info_generator <tuple_element <I>> (std::get <I> (args)).synthesize();

		if constexpr (I + 1 >= count) {
			if constexpr (I == 0)
				static_assert(false, "recursion detected in type info generation");

			if (current.is <primitive_type> ()) {
				auto &p = current.as <primitive_type> ();
				auto idx = em.emit_type_information(-1, -1, p.primitive);
				return struct_type(idx);
			} else {
				auto &s = current.as <struct_type> ();
				auto idx = em.emit_type_information(s.idx, -1, thunder::bad);
				return struct_type(idx);
			}
		} else {
			intermediate_type previous = partial <I + 1> ();

			auto &sp = previous.as <struct_type> ();
			
			if (current.is <primitive_type> ()) {
				auto &p = current.as <primitive_type> ();
				auto idx = em.emit_type_information(-1, sp.idx, p.primitive);
				return struct_type(idx);
			} else {
				auto &s = current.as <struct_type> ();
				auto idx = em.emit_type_information(s.idx, sp.idx, thunder::bad);
				return struct_type(idx);
			}
		}
	}

	intermediate_type synthesize() {
		return partial <0> ();
	}
};

template <native T>
struct type_info_generator <native_t <T>> {
	type_info_generator(const native_t <T> &) {}

	intermediate_type synthesize() {
		return primitive_type(native_t <T> ::primitive());
	}
};

template <native T, size_t D>
struct type_info_generator <vec <T, D>> {
	type_info_generator(const vec <T, D> &) {}

	intermediate_type synthesize() {
		return primitive_type(vec <T, D> ::primitive());
	}
};

int main()
{
	// f.dump();
	// fmt::println("{}", link(f).generate_glsl());
	// link(f).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);

	{
		thunder::Buffer buffer;

		auto &em = Emitter::active;
		
		em.push(buffer);

		type_info_generator(vec3(), vec4(), i32()).synthesize();

		em.pop();

		buffer.dump();
	}
}
