#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/ad/core.hpp"
#include "ire/emitter.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/opt.hpp"
#include "thunder/enumerations.hpp"
#include "wrapped_types.hpp"

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

void opt_transform_compact(Scratch &result, const Scratch &source)
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

		result.pool[i].reindex(relocation);
		// auto &&addrs = result.pool[i].addresses();
		// if (addrs.a0 != -1) relocation(addrs.a0);
		// if (addrs.a1 != -1) relocation(addrs.a1);

		doubled.emit(result.pool[i]);
	}

	std::swap(result, doubled);

	// TODO: do this step in place, without creating a duplicate buffer
	// just mark the unused  for dead-code-elimination later...
}

void opt_transform_constructor_elision(Scratch &result)
{
	// Find places where load from a constructed struct's field
	// can be skipped by simply forwarding the result it was
	// constructed with; if the constructor is completely useless,
	// then it will be removed during dead code elimination
				
	auto constructor_elision = [&](const std::vector <index_t> &fields,
				       const Load &ld,
				       index_t user) {
		if (ld.idx == -1)
			return;

		usage_list loaders = usage(result, user);

		wrapped::reindex <index_t> relocation;
		for (index_t i = 0; i < result.pointer; i++)
			relocation[i] = i;

		relocation[user] = fields[ld.idx];

		for (auto i : loaders)
			result.pool[i].reindex(relocation);
	};

	for (index_t i = 0; i < result.pointer; i++) {
		// Find all the construct calls
		auto &atom = result.pool[i];
		if (!atom.is <Construct> ())
			continue;

		// Gather the arguments
		// NOTE: this assumes that the constructor is a
		// plain initializer list and that it is in order
		Construct ctor = atom.as <Construct> ();

		std::vector <index_t> fields;

		index_t arg = ctor.args;
		while (arg != -1) {
			auto &atom = result.pool[arg];
			assert(atom.is <List> ());

			const List &list = atom.as <List> ();
			fields.push_back(list.item);
			arg = list.next;
		}

		usage_list users = usage(result, i);

		for (auto user : users) {
			auto &atom = result.pool[user];
			if (atom.is <Load> ())
				constructor_elision(fields, atom.as <Load> (), user);

			// TODO: if we get a store instruction,
			// we should stop...
		}
	}
}

// TODO: opt_transform_dead_code_elimination(...)

}

// Sandbox application
f32 __id(f32 x, f32 y)
{
	// return sin(x * x);
	return (x / y) + x;
}

auto id = callable(__id).named("id");

int main()
{
	Callable optimized;

	auto did = dfwd(id);

	// did.dump();
	thunder::opt_transform_compact(optimized, did);
	thunder::opt_transform_compact(optimized, Callable(optimized));
	
	optimized.dump();
	thunder::opt_transform_constructor_elision(optimized);

	optimized.dump();

	Callable original = did;

	fmt::println("{}", original.export_to_kernel().synthesize(profiles::opengl_450));
	fmt::println("{}", optimized.export_to_kernel().synthesize(profiles::opengl_450));
}
