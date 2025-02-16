#include "thunder/optimization.hpp"

namespace jvl::thunder {

[[gnu::always_inline]]
inline bool uses(Atom atom, Index i)
{
	auto &&addresses = atom.addresses();
	return (i != -1) && ((addresses.a0 == i) || (addresses.a1 == i));
}

usage_set usage(const std::vector <Atom> &pool, Index index)
{
	usage_set indices;
	for (size_t i = index + 1; i < pool.size(); i++) {
		if (uses(pool[i], index))
			indices.insert(i);
	}

	return indices;
}

usage_set usage(const Buffer &scratch, Index index)
{
	usage_set indices;
	for (size_t i = index + 1; i < scratch.pointer; i++) {
		if (uses(scratch.atoms[i], index))
			indices.insert(i);
	}

	return indices;
}

usage_graph usage(const Buffer &scratch)
{
	usage_graph graph(scratch.pointer);
	for (size_t i = 0; i < graph.size(); i++)
		graph[i] = usage(scratch, i);

	return graph;
}

} // namespace jvl::thunder
