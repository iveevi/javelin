#include <queue>
#include <unordered_set>

#include "ire/scratch.hpp"
#include "thunder/opt.hpp"

namespace jvl::thunder {

bool opt_transform_compact(ire::Scratch &result)
{
	bool marked = false;

	wrapped::reindex <index_t> relocation;
	for (index_t i = 0; i < result.pointer; i++) {
		if (relocation.contains(i))
			continue;

		Atom atom = result.pool[i];

		// Exhaustive search through other items
		for (index_t j = i + 1; j < result.pointer; j++) {
			if (relocation.contains(j))
				continue;

			Atom other = result.pool[j];
			if (atom == other) {
				relocation[j] = i;
				marked = true;
			}
		}

		// Fallback to itself
		relocation[i] = i;
	}

	// Reindex the relevant parts
	for (index_t i = 0; i < result.pointer; i++)
		result.pool[i].reindex(relocation);

	return marked;
}

bool opt_transform_constructor_elision(ire::Scratch &result)
{
	bool shortened = false;

	// Find places where load from a constructed struct's field
	// can be skipped by simply forwarding the result it was
	// constructed with; if the constructor is completely useless,
	// then it will be removed during dead code elimination

	auto constructor_elision = [&](const std::vector <index_t> &fields,
				       index_t idx,
				       index_t user) {
		if (idx == -1)
			return;

		usage_list loaders = usage(result, user);

		wrapped::reindex <index_t> relocation;
		for (index_t i = 0; i < result.pointer; i++)
			relocation[i] = i;

		relocation[user] = fields[idx];

		for (auto i : loaders)
			result.pool[i].reindex(relocation);

		shortened = true;
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
				constructor_elision(fields, atom.as <Load> ().idx, user);
			if (atom.is <Swizzle> ())
				constructor_elision(fields, atom.as <Swizzle> ().code, user);

			// TODO: if we get a store instruction,
			// we should stop...
		}
	}

	return shortened;
}

bool opt_transform_dce_exempt(const Atom &atom)
{
	return atom.is <Returns> () || atom.is <Store> ();
}

bool opt_transform_dead_code_elimination(ire::Scratch &result)
{
	usage_graph graph = usage(result);

	// Reversed usage graph
	usage_graph reversed(graph.size());
	for (index_t i = 0; i < graph.size(); i++) {
		for (index_t j : graph[i])
			reversed[j].insert(i);
	}

	// Configure checking queue and inclusion mask
	std::queue <index_t> check_list;
	std::vector <bool> include;

	for (index_t i = 0; i < result.pointer; i++) {
		check_list.push(i);
		include.push_back(true);
	}

	// Keep checking as long as something got erased
	while (check_list.size()) {
		std::unordered_set <index_t> erasure;

		while (check_list.size()) {
			index_t i = check_list.front();
			check_list.pop();

			auto &atom = result.pool[i];
			bool exempt = opt_transform_dce_exempt(atom);
			if (graph[i].empty() && !exempt) {
				include[i] = false;
				erasure.insert(i);
			}
		}

		fmt::println("  DCE pass eliminated {} atoms", erasure.size());

		std::unordered_set <index_t> reinsert;
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
	index_t pointer = 0;

	wrapped::reindex <index_t> relocation;
	for (index_t i = 0; i < result.pointer; i++) {
		if (include[i])
			relocation[i] = pointer++;
	}

	ire::Scratch doubled;
	for (index_t i = 0; i < result.pointer; i++) {
		if (relocation.contains(i)) {
			result.pool[i].reindex(relocation);
			doubled.emit(result.pool[i]);
		}
	}

	std::swap(result, doubled);

	// Change happened only if there is a difference in size
	return (result.pointer != doubled.pointer);
}

void opt_transform(ire::Scratch &result)
{
	bool changed;
	do {
		fmt::println("optimization pass (current # of atoms: {})", result.pointer);
		
		// Relinking steps, will not elimination code
		thunder::opt_transform_compact(result);
		thunder::opt_transform_constructor_elision(result);

		// Eliminate unused code
		changed = thunder::opt_transform_dead_code_elimination(result);
	} while (changed);
}

} // namespace jvl::thunder
