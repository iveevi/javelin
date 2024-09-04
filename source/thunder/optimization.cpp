#include "thunder/opt.hpp"

namespace jvl::thunder {

void opt_transform_compact(ire::Scratch &result)
{
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
			if (atom == other)
				relocation[j] = i;
		}

		// Fallback to itself
		relocation[i] = i;
	}

	// Reindex the relevant parts
	for (index_t i = 0; i < result.pointer; i++)
		result.pool[i].reindex(relocation);
}

void opt_transform_constructor_elision(ire::Scratch &result)
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

bool opt_transform_dce_exempt(const Atom &atom)
{
	return atom.is <Returns> () || atom.is <Store> ();
}

void opt_transform_dead_code_elimination(ire::Scratch &result)
{
	usage_graph graph = usage(result);

	index_t pointer = 0;

	wrapped::reindex <index_t> relocation;
	for (index_t i = 0; i < result.pointer; i++) {
		auto &atom = result.pool[i];
		bool exempt = opt_transform_dce_exempt(atom);
		if (graph[i].size() || exempt)
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
}

} // namespace jvl::thunder
