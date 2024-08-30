#include "ire/scratch.hpp"
#include "thunder/opt.hpp"

namespace jvl::thunder {

[[gnu::always_inline]]
inline bool uses(Atom atom, index_t i)
{
	auto &&addresses = atom.addresses();
	return (i != -1) && ((addresses.a0 == i) || (addresses.a1 == i));
}

usage_list usage(const std::vector <Atom> &pool, index_t index)
{
	usage_list indices;
	for (index_t i = index + 1; i < pool.size(); i++) {
		if (uses(pool[i], index))
			indices.push_back(i);
	}

	return indices;
}

usage_list usage(const ire::Scratch &scratch, index_t index)
{
	usage_list indices;
	for (index_t i = index + 1; i < scratch.pointer; i++) {
		if (uses(scratch.pool[i], index))
			indices.push_back(i);
	}

	return indices;
}

usage_graph usage(const ire::Scratch &scratch)
{
	usage_graph graph(scratch.pointer);
	for (index_t i = 0; i < graph.size(); i++)
		graph[i] = usage(scratch, i);

	return graph;
}

} // namespace jvl::thunder