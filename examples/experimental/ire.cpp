// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct RayFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return layout_from("RayFrame",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical));
	}
};

// TODO: aggregates with unsized array

// TODO: if not a buffer, reroute to buffer...
template <generic T>
struct buffer_reference : tagged {
	// TODO: name hint for this
	buffer_reference() {
		auto &em = Emitter::active;

		auto v = T();
		auto type = type_info_generator <T> (v)
			.synthesize()
			.concrete();

		// TODO: numerical should be the unique index...
		auto qual = em.emit_qualifier(type, -1, thunder::buffer_reference);

		this->ref = qual;
	}

	T operator()(const native_t <uint64_t> &x) {
		auto &em = Emitter::active;

		auto list = em.emit_list(x.synthesize().id);
		auto value = em.emit_construct(this->ref.id, list, thunder::normal);

		// TODO: case by case for builtin
		return cache_index_t::from(value);
	}
};

auto ftn = procedure <void> ("main") << []()
{
	write_only <scalar <buffer <unsized_array <vec3>>>> bf(0);

	// // read_only <buffer <unsized_array <vec3>>> bf(0);
	// bf[1].x = 45;

	// auto temporary = scalar <buffer_reference <ivec2>> ();

	// TODO: ensure no duplicates...
	auto temporary = buffer_reference <ivec2> ();

	u64 x = 0;
	// x = 12;

	auto b = temporary(x);

	bf[b.y] = vec3(1.04);
};

// TODO: shadertoy example

int main()
{
	ftn.dump();
	dump_lines("EXPERIMENTAL IRE", link(ftn).generate_glsl());
	link(ftn).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
