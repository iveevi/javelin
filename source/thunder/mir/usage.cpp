#include "thunder/mir/transformations.hpp"

namespace jvl::thunder::mir {

bestd::tree <Index, Set> mole_usage(const Block &block)
{
	bestd::tree <Index, Set> graph;

	for (auto &p : block.body) {
		auto ftn = [&](auto x) { return addresses(x); };
		graph[p.idx()] = std::visit(ftn, *p);
	}

	return graph;
}

bestd::tree <Index, Set> mole_users(const Block &block)
{
	bestd::tree <Index, Set> graph;

	for (auto &p : block.body) {
		auto ftn = [&](auto x) { return addresses(x); };
		auto set = std::visit(ftn, *p);
		for (auto v : set)
			graph[v].insert(p.index);
	}

	return graph;
}

} // namespace jvl::thunder::mir