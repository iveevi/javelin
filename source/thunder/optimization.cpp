#include <queue>
#include <unordered_set>

#include "core/reindex.hpp"
#include "core/logging.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/optimization.hpp"

namespace jvl::thunder {

MODULE(optimization);

bool optimize_compaction(Buffer &result)
{
	bool marked = false;

	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++) {
		if (relocation.contains(i))
			continue;

		Atom atom = result.atoms[i];

		// Calls can mutate the state,
		// deal with this in linkage
		bool enable = !atom.is <Call> ();

		// Exhaustive search through other items
		for (size_t j = i + 1; enable && (j < result.pointer); j++) {
			if (relocation.contains(j))
				continue;

			Atom other = result.atoms[j];
			if (atom == other) {
				relocation[j] = i;
				marked = true;
			}
		}

		// Fallback to itself
		relocation[i] = i;
	}

	// Reindex the relevant parts
	for (size_t i = 0; i < result.pointer; i++)
		result.atoms[i].reindex(relocation);

	return marked;
}

bool optimize_constructor_elision(Buffer &result)
{
	// Find places where load from a constructed struct's field
	// can be skipped by simply forwarding the result it was
	// constructed with; if the constructor is completely useless,
	// then it will be removed during dead code elimination

	auto graph = usage(result);

	auto constructor_elision = [&](const std::vector <Index> &fields,
				       Index idx,
				       Index user) -> bool {
		if (idx == -1)
			return false;

		usage_set loaders = graph[user];

		reindex <Index> relocation;
		for (size_t i = 0; i < result.pointer; i++)
			relocation[i] = i;

		relocation[user] = fields[idx];

		for (auto i : loaders)
			result.atoms[i].reindex(relocation);

		return true;
	};

	bool shortened = false;
	for (size_t i = 0; i < result.pointer; i++) {
		// Find all the construct calls
		auto &atom = result.atoms[i];
		if (!atom.is <Construct> ())
			continue;

		// Gather the arguments
		// NOTE: this assumes that the constructor is a
		// plain initializer list and that it is in order
		Construct construct = atom.as <Construct> ();
		if (construct.mode == transient)
			continue;

		Index arg = construct.args;
		if (arg == -1)
			continue;

		std::vector <Index> fields;
		while (arg != -1) {
			auto &atom = result.atoms[arg];
			JVL_ASSERT_PLAIN(atom.is <List> ());

			const List &list = atom.as <List> ();
			fields.push_back(list.item);
			arg = list.next;
		}

		// TODO: need to check for stores that happen in between...

		for (auto user : graph[i]) {
			auto &atom = result.atoms[user];
			if (atom.is <Load> ())
				shortened |= constructor_elision(fields, atom.as <Load> ().idx, user);
			if (atom.is <Swizzle> ())
				shortened |= constructor_elision(fields, atom.as <Swizzle> ().code, user);

			// TODO: if we get a store instruction,
			// we should stop...
		}
	}

	return shortened;
}

bool opt_transform_dce_exempt(const Atom &atom)
{
	static const std::set <thunder::IntrinsicOperation> intrinsic_exempt {
		thunder::layout_local_size,
		thunder::layout_mesh_shader_sizes,
		thunder::emit_mesh_tasks,
		thunder::set_mesh_outputs,
		thunder::glsl_barrier,
	};

	if (auto intrinsic = atom.get <Intrinsic> ())
		return intrinsic_exempt.contains(intrinsic->opn);

	return atom.is <Returns> ()
		|| atom.is <Store> ()
		|| atom.is <Qualifier> ()
		|| atom.is <Branch> ();
}

bool opt_transform_dce_override(const Atom &atom)
{
	// Even if the atom is exempt, it may be subject to removal
	switch (atom.index()) {
	variant_case(Atom, Store):
	{
		// Check for spurious no-op stores
		auto &store = atom.as <Store> ();
		return store.dst == store.src;
	}

	default:
		break;
	}

	return false;
}

bool optimize_dead_code_elimination(Buffer &result)
{
	usage_graph graph = usage(result);

	// Reversed usage graph
	usage_graph reversed(graph.size());
	for (size_t i = 0; i < graph.size(); i++) {
		for (Index j : graph[i])
			reversed[j].insert(i);
	}

	// Configure checking queue and inclusion mask
	std::queue <Index> check_list;
	std::vector <bool> include;

	for (size_t i = 0; i < result.pointer; i++) {
		check_list.push(i);
		include.push_back(true);
	}

	// Keep checking as long as something got erased
	while (check_list.size()) {
		std::unordered_set <Index> erasure;

		while (check_list.size()) {
			Index i = check_list.front();
			check_list.pop();

			auto &atom = result.atoms[i];

			bool exempt = opt_transform_dce_exempt(atom);
			exempt &= !opt_transform_dce_override(atom);

			if (graph[i].empty() && !exempt) {
				include[i] = false;
				erasure.insert(i);
			}
		}

		std::unordered_set <Index> reinsert;
		for (auto i : erasure) {
			for (auto j : reversed[i]) {
				graph[j].erase(i);
				reinsert.insert(j);
			}
		}

		for (auto j : reinsert)
			check_list.push(j);
	}

	// Reconstruct with the reduced set
	Index pointer = 0;

	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++) {
		if (include[i])
			relocation[i] = pointer++;
	}

	Buffer doubled;
	for (size_t i = 0; i < result.pointer; i++) {
		if (relocation.contains(i)) {
			result.atoms[i].reindex(relocation);
			doubled.emit(result.atoms[i]);
		}
	}

	doubled.decorations = result.decorations;

	for (auto &[i, j] : result.used_decorations) {
		auto k = relocation[i];
		doubled.used_decorations[k] = j;
	}

	std::swap(result, doubled);

	// Change happened only if there is a difference in size
	return (result.pointer != doubled.pointer);
}

void optimize(Buffer &result)
{
	JVL_STAGE();

	bool changed;
	do {
		JVL_INFO("looped optimization pass (current # of atoms: {})", result.pointer);

		// Relinking steps, will not elimination code
		thunder::optimize_compaction(result);

		// Disable due to incorrectness
		// thunder::opt_transform_constructor_elision(result);
		// TODO: constant propagation

		// Eliminate unused code
		changed = thunder::optimize_dead_code_elimination(result);
	} while (changed);
}

} // namespace jvl::thunder
