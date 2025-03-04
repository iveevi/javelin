#include <queue>
#include <unordered_set>

#include "core/reindex.hpp"
#include "core/logging.hpp"
#include "thunder/optimization.hpp"

namespace jvl::thunder {

MODULE(optimization);

bool optimize_dead_code_elimination_iteration(Buffer &result)
{
	usage_graph graph = usage(result);

	// Reversed usage graph
	// TODO: at most 3 refs per atom, so use an array?
	usage_graph reversed(graph.size());
	for (size_t i = 0; i < graph.size(); i++) {
		for (Index j : graph[i])
			reversed[j].insert(i);
	}

	// Configure checking queue and inclusion mask
	std::vector <bool> include(result.pointer, true);
	
	std::queue <Index> check_list;
	for (size_t i = 0; i < result.pointer; i++)
		check_list.push(i);

	// Keep checking as long as something got erased
	while (check_list.size()) {
		std::unordered_set <Index> erasure;

		while (check_list.size()) {
			Index i = check_list.front();
			check_list.pop();
			
			if (graph[i].empty() && !result.synthesized.contains(i)) {
				include[i] = false;
				erasure.insert(i);
			}
		}

		for (auto i : erasure) {
			for (auto j : reversed[i]) {
				graph[j].erase(i);
				check_list.push(j);
			}
		}
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

	// Transfer decorations
	doubled.decorations.all = result.decorations.all;

	for (auto &[i, j] : result.decorations.used) {
		auto k = relocation[i];
		doubled.decorations.used[k] = j;
	}
	
	for (auto &i : result.decorations.phantom) {
		auto k = relocation[i];
		doubled.decorations.phantom.insert(k);
	}

	std::swap(result, doubled);

	// Change happened only if there is a difference in size
	return (result.pointer != doubled.pointer);
}

bool optimize_dead_code_elimination(Buffer &result)
{
	uint32_t counter = 0;
	uint32_t size_begin = result.pointer;

	JVL_STAGE();

	bool changed;
	do {
		changed = optimize_dead_code_elimination_iteration(result);
		counter++;
	} while (changed);

	uint32_t size_end = result.pointer;
	
	JVL_INFO("ran dead code elimination pass {} times, reduced size from {} to {}", counter, size_begin, size_end);

	return size_begin != size_end;
}

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

	for (size_t i = 0; i < result.pointer; i++) {
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

void optimize(Buffer &result)
{
	uint32_t counter = 0;

	bool changed;

	do {
		changed = optimize_dead_code_elimination(result);
		changed |= optimize_deduplicate(result);
		counter++;
	} while (changed);

	JVL_INFO("ran full optimization pass {} times", counter);
}

} // namespace jvl::thunder
