#include "thunder/atom.hpp"
#include "thunder/linkage.hpp"
#include "logging.hpp"

namespace jvl::thunder::detail {

MODULE(synthesized);

index_t reference_of(const std::vector <Atom> &atoms, index_t i)
{
	auto &atom = atoms[i];

	switch (atom.index()) {
	case Atom::type_index <Swizzle> ():
		return reference_of(atoms, atom.as <Swizzle> ().src);
	case Atom::type_index <Load> ():
		return reference_of(atoms, atom.as <Load> ().src);
	default:
		return i;
	}
}

std::unordered_set <index_t> synthesize_list(const std::vector <Atom> &atoms)
{
	std::unordered_set <index_t> synthesized;

	// Find all the critical instructions
	for (index_t i = 0; i < atoms.size(); i++) {
		auto &atom = atoms[i];

		switch (atom.index()) {

		case Atom::type_index <Store> ():
		{
			index_t dst = atom.as <Store> ().dst;
			synthesized.insert(i);
			synthesized.insert(reference_of(atoms, dst));
			break;
		}

		case Atom::type_index <Branch> ():
			synthesized.insert(i);
			break;

		case Atom::type_index <Qualifier> ():
		{
			auto &global = atom.as <Qualifier> ();
			synthesized.insert(i);
			synthesized.insert(global.underlying);
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

		case Atom::type_index <TypeInformation> ():
		{
			auto &type_field = atom.as <TypeInformation> ();
			if (type_field.down != -1)
				synthesized.insert(type_field.down);
		} break;

		}
	}

	return synthesized;
}

} // namespace jvl::thunder::detail
