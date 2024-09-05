#include "thunder/linkage.hpp"

namespace jvl::thunder::detail {

std::unordered_set <index_t> synthesize_list(const std::vector <Atom> &atoms)
{
	std::unordered_set <index_t> synthesized;

	// Find all the critical instructions
	for (index_t i = 0; i < atoms.size(); i++) {
		auto &atom = atoms[i];
		if (atom.is <Returns> () || atom.is <Store> ())
			synthesized.insert(i);
	}

	return synthesized;
}

} // namespace jvl::thunder::detail