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

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: get line numbers for each invocation if possible?
// using source location if available

Procedure pcg3d = procedure("pcg3d") << [](uvec3 v) -> uvec3
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
};

Procedure random3 = procedure("random3") << [](inout <vec3> seed) -> vec3
{
	seed = uintBitsToFloat((pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	return seed;
};

Procedure spherical = procedure("spherical") << [](f32 theta, f32 phi) -> vec3
{
	return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
};

Procedure randomH2 = procedure("randomH2") << [](inout <vec3> seed) -> vec3
{
	// TODO: fix
	vec3 eta = random3(seed);
	f32 theta = acos(eta.x);
	f32 phi = float(2 * M_PI) * eta.y;
	return spherical(theta, phi);
};

namespace jvl::thunder {

bool casting_intrinsic(const Intrinsic &intr)
{
	switch (intr.opn) {
	case cast_to_float:
		return true;
	default:
		break;
	}

	return false;
}

// TODO: optimizer class...
bool optimize_casting_elision(Buffer &result)
{
	bool changed = false;

	usage_graph graph = usage(result);
	
	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++)
		relocation[i] = i;

	for (Index i = 0; i < result.pointer; i++) {
		auto &atom = result.atoms[i];

		if (atom.is <Construct> ()) {
			// Constructors with one argument of the same type
			auto &ctor = atom.as <Construct> ();
			if (ctor.mode != normal)
				continue;

			auto oqt = result.types[i];
			auto aqt = QualifiedType();
			
			auto &args = result.atoms[ctor.args];
			JVL_ASSERT(args.is <List> (), "constructor arguments should be a list chain");

			auto &list = args.as <List> ();

			// Only handle singletons
			if (list.next != -1)
				continue;

			aqt = result.types[list.item];
			if (oqt == aqt) {
				relocation[i] = list.item;
				result.synthesized.erase(i);
			}
		} else if (atom.is <Intrinsic> ()) {
			auto &intr = atom.as <Intrinsic> ();
			if (!casting_intrinsic(intr))
				continue;

			// Casting intrinsics with the same type
			fmt::println("potentially optimizable: {}", intr.to_assembly_string());
			
			// TODO: this is the same code as for the constructor at the end...
			auto oqt = result.types[i];
			auto aqt = QualifiedType();
			
			auto &args = result.atoms[intr.args];
			JVL_ASSERT(args.is <List> (), "constructor arguments should be a list chain");

			auto &list = args.as <List> ();

			// Only handle singletons
			if (list.next != -1)
				continue;

			aqt = result.types[list.item];
			if (oqt == aqt) {
				relocation[i] = list.item;
				result.synthesized.erase(i);
			}
		}
	}

	// TODO: refine relocation
	for (auto &[k, v] : relocation) {
		auto pv = v;

		do {
			pv = v;
			relocation(v);
		} while (pv != v);
	}
 
	fmt::println("relocation:");
	for (auto [k, v] : relocation)
		fmt::println("\t{} -> {}", k, v);
	
	for (size_t i = 0; i < result.pointer; i++)
		result.atoms[i].reindex(relocation);

	return changed;
}

} // namespace jvl::thunder

// TODO: promote atoms to synthesizable if used multiple times
int main()
{
	thunder::optimize(pcg3d);
	thunder::optimize(random3);
	thunder::optimize(spherical);
	thunder::optimize(randomH2);

	thunder::optimize_casting_elision(randomH2);
	thunder::optimize_dead_code_elimination(randomH2);
	randomH2.graphviz("randomH2.dot");
	
	spherical.graphviz("spherical.dot");

	auto unit = link(randomH2);

	dump_lines("RANDOM H2", unit.generate_glsl());

	unit.write_assembly("ire.jvl.asm");

	// unit.generate_spirv_via_glsl(vk::ShaderStageFlagBits::eCompute);
}