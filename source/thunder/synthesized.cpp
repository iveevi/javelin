#include "thunder/atom.hpp"
#include "thunder/linkage.hpp"

namespace jvl::thunder::detail {

std::unordered_set <index_t> synthesize_list(const std::vector <Atom> &atoms)
{
	std::unordered_set <index_t> synthesized;

	// Find all the critical instructions
	for (index_t i = 0; i < atoms.size(); i++) {
		auto &atom = atoms[i];

		switch (atom.index()) {

		case Atom::type_index <Store> ():
			synthesized.insert(i);
			synthesized.insert(atom.as <Store> ().dst);
			break;

		case Atom::type_index <Branch> ():
		case Atom::type_index <End> ():
			synthesized.insert(i);
			break;

		case Atom::type_index <Global> ():
		{
			auto &global = atom.as <Global> ();
			synthesized.insert(i);
			synthesized.insert(global.type);
		} break;

		case Atom::type_index <Returns> ():
		{
			auto &returns = atom.as <Returns> ();
			synthesized.insert(i);

			if (returns.type != -1)
				synthesized.insert(returns.type);
		} break;

		case Atom::type_index <Operation> ():
			synthesized.insert(atom.as <Operation> ().type);
			break;

		case Atom::type_index <TypeField> ():
		{
			auto &type_field = atom.as <TypeField> ();
			if (type_field.down != -1)
				synthesized.insert(type_field.down);
		} break;

		}
	}

	return synthesized;
}

} // namespace jvl::thunder::detail
