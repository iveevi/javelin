#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/scratch.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types

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

	f32 has_albedo;
	f32 has_normal;

	auto layout() const {
		return uniform_layout(
			"Material",
			field <"diffuse"> (diffuse),
			field <"specular"> (specular),
			field <"emission"> (emission),
			field <"ambient"> (ambient),
			field <"shininess"> (shininess),
			field <"roughness"> (roughness),
			field <"has_albedo"> (has_albedo),
			field <"has_normal"> (has_normal)
		);
	}
};

// Random number generation
uvec3 __pcg3d(uvec3 v)
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	// v.y += v.z * v.x;
	// v.z += v.x * v.y;
	// v ^= v >> 16u;
	// v.x += v.y * v.z;
	// v.y += v.z * v.x;
	// v.z += v.x * v.y;
	return v;
}

auto pcg3d = callable(__pcg3d).named("pcg3d");

namespace jvl::thunder {

struct ref_index_t {
	index_t index;
	int8_t mask = 0b11;
};

struct mapped_inst_t {
	ire::Scratch transformed;
	std::vector <ref_index_t> refs;

	Atom &operator[](size_t index) {
		return transformed.pool[index];
	}

	void track(index_t index, int8_t mask = 0b11) {
		refs.push_back(ref_index_t(index, mask));
	}
};

PrimitiveType swizzle_type_of(PrimitiveType primitive, SwizzleCode code)
{
	// TODO: might have to handle multiswizzle
	switch (primitive) {
	case ivec2:
	case ivec3:
	case ivec4:
		return i32;
	case uvec2:
	case uvec3:
	case uvec4:
		return u32;
	case vec2:
	case vec3:
	case vec4:
		return f32;
	default:
		break;
	}

	fmt::println("unhandled case for swizzle_type_of: {}", tbl_primitive_types[primitive]);
	abort();
}

PrimitiveType primitive_type_of(const std::vector <Atom> &pool, index_t i)
{
	auto &atom = pool[i];

	switch (atom.index()) {
	case Atom::type_index <Global> ():
		return primitive_type_of(pool, atom.as <Global> ().type);
	case Atom::type_index <TypeField> ():
		return atom.as <TypeField> ().item;
	case Atom::type_index <Primitive> ():
		return atom.as <Primitive> ().type;
	case Atom::type_index <Operation> ():
		return primitive_type_of(pool, atom.as <Operation> ().type);
	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();
		return swizzle_type_of(primitive_type_of(pool, swz.src), swz.code);
	}
	default:
		break;
	}

	fmt::println("unhandled case for primitive_type_of: {}", atom);
	abort();
}

void legalize_for_jit(ire::Scratch &scratch)
{
	auto &em = ire::Emitter::active;
	auto &pool = scratch.pool;

	std::vector <mapped_inst_t> mapped(scratch.pointer);
	
	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = pool[i];

		// Default population of scratches is preservation
		em.push(mapped[i].transformed);
		em.emit(atom);
		em.pop();
	}

	auto list_args = [&](index_t i) {
		// TODO: util function
		std::vector <index_t> args;
		while (i != -1) {
			auto &atom = pool[i];
			assert(atom.is <List> ());

			List list = atom.as <List> ();
			args.push_back(list.item);

			i = list.next;
		}

		return args;
	};

	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = pool[i];

		// If its a binary operation, we need to ensure
		// that the overload is legalized, at this stage
		// all the operations must have been validated
		// according to the GLSL specification
		if (auto op = atom.get <Operation> ()) {
			// The operands are guaranteed to be
			// primitive types at this point
			fmt::println("binary operation: {}", atom);

			auto args = list_args(op->args);
			fmt::println("type for each args:");
			for (auto i : args) {
				auto ptype = primitive_type_of(pool, i);
				fmt::println("  {} -> {}", i, tbl_primitive_types[ptype]);
			}

			if (args[0] != args[1]) {
				fmt::println("unsupported operation overload, need to legalize");
				assert(false);
			}
		}
	}
}

}

int main()
{
	thunder::opt_transform_compact(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	thunder::opt_transform_dead_code_elimination(pcg3d);
	pcg3d.dump();

	thunder::legalize_for_jit(pcg3d);
	pcg3d.dump();

	// jit(pcg3d);
}