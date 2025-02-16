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

inline int64_t type_index_counter = 0;

template <typename T>
int64_t type_index()
{
	// Force ID fetch at runtime
	static int64_t c = -1;
	if (c == -1)
		c = type_index_counter++;
	return c;
}

template <generic ... Args>
struct aggregate_wrapper {};

struct primitive_type {
	thunder::PrimitiveType ptv;
};

struct struct_type {
	thunder::Index idx;
};

using intermediate_type = bestd::variant <primitive_type, struct_type>;

// Forward declarations
template <typename T>
struct type_info_generator {};

namespace detail {

template <string_literal str, generic T>
struct field {
	using underlying = std::decay_t <T>;

	static constexpr const char *name = str.value;

	T &ref;

	field(T &val) : ref(val) {}
};

template <typename T>
struct is_field_type : std::false_type {};

template <string_literal str, generic T>
struct is_field_type <field <str, T>> : std::true_type {};

template <typename T>
concept field_type = is_field_type <T> ::value;

template <string_literal str, field_type ... Fields>
struct Layout;

template <typename T>
struct is_layout_type : std::false_type {};

template <string_literal str, field_type ... Fields>
struct is_layout_type <Layout <str, Fields...>> : std::true_type {};

template <typename T>
concept layout_type = is_layout_type <T> ::value;

template <typename T>
concept aggregate = requires(T &t) {
	{ t.ll() } -> layout_type;
};

template <string_literal str, field_type ... Fields>
struct Layout {
	using T = aggregate_wrapper <typename Fields::underlying...>;

	std::tuple <Fields...> references;

	template <typename ... Args>
	Layout(Args &... args) : references(args...) {}

	void embed_hint(thunder::Index src) {
		auto &em = Emitter::active;

		// TODO: evaluate once statically?
		std::vector <std::string> fields = std::apply([](const auto &... fields) {
			return std::vector <std::string> { fields.name... };
		}, references);

		em.emit_hint(src, type_index <T> (), str.value, fields);
	}

	template <size_t I = 0>
	void link(thunder::Index src) {
		auto &em = Emitter::active;
		auto &ref = std::get <I> (references).ref;

		using E = decltype(ref);

		// TODO: aggregates
		auto idx = em.emit_load(src, thunder::Index(I));
		if constexpr (builtin <E>)
			ref.ref = cache_index_t::from(idx);
		else if constexpr (aggregate <E>)
			ref.ll().link(idx);

		if constexpr (I + 1 < sizeof...(Fields))
			link <I + 1> (src);

		if constexpr (I == 0)
			embed_hint(src);
	}
	
	intermediate_type generate_type() {	
		auto &em = Emitter::active;

		intermediate_type idx = std::apply([](const auto &... fields) {
			return type_info_generator <T> (fields.ref...).synthesize();
		}, references);

		JVL_ASSERT(idx.is <struct_type> (), "layout type info generation is not a struct type");

		embed_hint(idx.as <struct_type> ().idx);

		return idx;
	}
};

}

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

	auto ll() {
		return ::detail::Layout <"RayFrame",
			::detail::field <"origin", vec3>,
			::detail::field <"lower_left", vec3>
		> (origin, lower_left);
	}
};

static_assert(::detail::aggregate <RayFrame>);

struct Nested {
	vec3 x;
	RayFrame frame;

	auto layout() const {
		return uniform_layout("Nested",
			named_field(x),
			named_field(frame));
	}
	
	auto ll() {
		return ::detail::Layout <"Nested",
			::detail::field <"x", vec3>,
			::detail::field <"frame", RayFrame>
		> (x, frame);
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

// General form is explicitly a struct with fields of type Args...
template <generic ... Args>
struct type_info_generator <aggregate_wrapper <Args...>> {
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
			if (current.is <primitive_type> ()) {
				auto &p = current.as <primitive_type> ();
				auto idx = em.emit_type_information(-1, -1, p.ptv);
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
				auto idx = em.emit_type_information(-1, sp.idx, p.ptv);
				return struct_type(idx);
			} else {
				auto &s = current.as <struct_type> ();
				auto idx = em.emit_type_information(s.idx, sp.idx, thunder::bad);
				return struct_type(idx);
			}
		}
	}

	// TODO: optionally pass type hints...
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

template <::detail::aggregate T>
struct type_info_generator <T> {
	T &ref;

	intermediate_type synthesize() {
		return ref.ll().generate_type();
	}
};

int main()
{
	// f.dump();
	// fmt::println("{}", link(f).generate_glsl());
	// link(f).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);

	{
		thunder::TrackedBuffer buffer;
		buffer.name = "main";

		auto &em = Emitter::active;
		
		em.push(buffer);

		RayFrame rayframe;
		auto type = type_info_generator <RayFrame> (rayframe).synthesize();
		auto idx = type.as <struct_type> ().idx;

		idx = em.emit_construct(idx, -1, thunder::normal);
		
		rayframe.ll().link(idx);

		f32 x;
		x = rayframe.origin.x;
		
		Nested nested;
		type = type_info_generator <Nested> (nested).synthesize();
		idx = type.as <struct_type> ().idx;
		
		idx = em.emit_construct(idx, -1, thunder::normal);

		nested.ll().link(idx);

		x = nested.frame.lower_left.y;
		// x = nested.x.y;

		em.pop();

		buffer.dump();

		fmt::println("{}", link(buffer).generate_glsl());

		link(buffer).generate_spirv(vk::ShaderStageFlagBits::eAll);
	}
}
