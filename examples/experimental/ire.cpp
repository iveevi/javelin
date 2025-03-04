// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct Data {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	unsized_array <vec3> data;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return phantom_layout_from("Data",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical),
			verbatim_field(data));
	}
};

// TODO: type check for soft/concrete types... layout_from only for concrete types

// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

struct Reference {
	u64 triangles;
	u64 vertices;

	auto layout() {
		return layout_from("Reference",
			verbatim_field(triangles),
			verbatim_field(vertices));
	}
};

auto ftn = procedure <void> ("main") << []()
{
	writeonly <scalar <buffer <unsized_array <vec3>>>> bf(0);

	buffer <unsized_array <Reference>> references(1);

	// bf[0] = vec3(10);

	// shared <array <vec4>> arr(16);
	// arr[0] = vec4(0);

	u64 x = references[0].vertices;
	auto br1 = scalar <buffer_reference <unsized_array <vec3>>> (x);
	bf[1] = br1[12];
	
	// auto br2 = buffer_reference <Data> (x);
	// arr[1] = vec4(br2.data[14], 1);
};

namespace jvl::thunder {

bool optimize_deduplicate_iteration(Buffer &result)
{
	static_assert(sizeof(Atom) == sizeof(uint64_t));
	
	bool changed = false;

	// Each atom converted to a 64-bit integer
	std::map <uint64_t, Index> existing;

	auto unique = [&](Index i) -> Index {
		auto &atom = result.atoms[i];

		// Handle exceptions
		if (atom.is <Store> () || atom.is <Call> ())
			return i;

		auto &hash = reinterpret_cast <uint64_t &> (atom);

		auto it = existing.find(hash);
		if (it != existing.end()) {
			if (i != it->second) {
				changed = true;
				result.synthesized.erase(i);
			}

			return it->second;
		}

		existing[hash] = i;

		return i;
	};

	for (Index i = 0; i < result.pointer; i++) {
		auto addresses = result.atoms[i].addresses();
		if (addresses.a0 != -1)
			addresses.a0 = unique(addresses.a0);
		if (addresses.a1 != -1)
			addresses.a1 = unique(addresses.a1);
	}

	return changed;
}

bool optimize_deduplicate(Buffer &result)
{
	uint32_t counter = 0;

	bool changed;

	do {
		changed = optimize_deduplicate_iteration(result);
		counter++;
	} while (changed);

	JVL_INFO("ran deduplication pass {} times", counter);

	return (counter > 1);
}

}

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: optimization using graphviz for visualization...
// TODO: get line numbers for each invocation if possible?

int main()
{
	// ftn.dump();
	ftn.graphviz("graph.dot");

	thunder::optimize_dead_code_elimination(ftn);
	thunder::optimize_deduplicate(ftn);
	thunder::optimize_dead_code_elimination(ftn);

	ftn.graphviz("graph-optimized.dot");

	// ftn.dump();

	// Computing mask for atoms
	{
		thunder::Atom type0 = thunder::TypeInformation(0, 0, thunder::PrimitiveType(0));
		thunder::Atom typeF = thunder::TypeInformation(0xFFFF, 0xFFFF, thunder::PrimitiveType(0xFFFF));
		fmt::println("type0: {:016x}", reinterpret_cast <uint64_t &> (type0));
		fmt::println("typeF: {:016x}", reinterpret_cast <uint64_t &> (typeF));
		uint64_t bits0 = reinterpret_cast <uint64_t &> (type0);
		uint64_t bitsF = reinterpret_cast <uint64_t &> (typeF);
		uint64_t mask = bitsF ^ bits0;
		fmt::println("mask: {:016x}", mask);
	}

	// link(ftn).generate(Target::glsl);
	// dump_lines("EXPERIMENTAL IRE", link(ftn).generate_glsl());
	// link(ftn).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eCompute);
}
