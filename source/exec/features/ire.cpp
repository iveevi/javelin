#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/ad/core.hpp"
#include "ire/emitter.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: pass structs as inout to callables
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

namespace jvl::thunder {

void opt_compact_transform(Scratch &result, const Scratch &source)
{
	// Always begin with a copy
	result = source;

	wrapped::reindex <index_t> reindex;
	for (index_t i = 0; i < source.pointer; i++) {
		if (reindex.contains(i))
			continue;

		Atom atom = source.pool[i];

		// Exhaustive search through other items
		for (index_t j = i + 1; j < source.pointer; j++) {
			if (reindex.contains(j))
				continue;

			Atom other = source.pool[j];
			if (atom == other)
				reindex[j] = i;
		}

		// Fallback to itself
		reindex[i] = i;
	}

	// Reindex the relevant parts
	for (index_t i = 0; i < result.pointer; i++) {
		auto &&addrs = result.pool[i].addresses();
		if (addrs.a0 != -1) reindex(addrs.a0);
		if (addrs.a1 != -1) reindex(addrs.a1);
	}

	// Recalculate the positions
	index_t pointer = 0;

	wrapped::reindex <index_t> relocation;
	for (index_t i = 0; i < result.pointer; i++) {
		if (reindex[i] != i) {
			relocation[i] = -1;
			continue;
		}

		relocation[i] = pointer++;
	}
	
	Scratch doubled;
	for (index_t i = 0; i < result.pointer; i++) {
		if (reindex[i] != i)
			continue;

		auto &&addrs = result.pool[i].addresses();
		if (addrs.a0 != -1) relocation(addrs.a0);
		if (addrs.a1 != -1) relocation(addrs.a1);

		doubled.emit(result.pool[i]);
	}

	std::swap(result, doubled);
}

}

// Sandbox application
f32 __id(f32 x, f32 y)
{
	return sin(x * x);
	// return (x / y) + x;
}

auto id = callable(__id).named("id");

int main()
{
	Scratch s;

	id.dump();
	thunder::opt_compact_transform(s, id);
	s.dump();

	auto did = dfwd(id);

	did.dump();
	thunder::opt_compact_transform(s, did);
	s.dump();

	auto shader = [&]() {
		layout_in <float> input(0);
		layout_out <float> output(0);

		dual_t <f32> dx = dual(id(input, 1), f32(1.0f));
		dual_t <f32> dy = dual(id(1, (f32) input / 2.0f), input);

		output = did(dx, dy).dual;
	};

	auto kernel = kernel_from_args(shader);

	fmt::println("{}", kernel.synthesize(profiles::opengl_450));
}
