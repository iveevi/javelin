#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/linkage.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

// MODULE(include-atoms);

index_t reference_of(const std::vector <Atom> &atoms, index_t i)
{
	auto &atom = atoms[i];

	switch (atom.index()) {
	case Atom::type_index <Swizzle> ():
		return reference_of(atoms, atom.as <Swizzle> ().src);
	case Atom::type_index <Load> ():
		return reference_of(atoms, atom.as <Load> ().src);
	case Atom::type_index <ArrayAccess> ():
		return reference_of(atoms, atom.as <ArrayAccess> ().src);
	default:
		return i;
	}
}

void Buffer::include(index_t i)
{
	auto &atom = atoms[i];

	switch (atom.index()) {

	// Store instructions and their (root)
	// destinations should always be synthesized
	case Atom::type_index <Store> ():
	{
		index_t dst = atom.as <Store> ().dst;
		synthesized.insert(i);
		synthesized.insert(reference_of(atoms, dst));
		break;
	}

	// Always synthesize branches
	case Atom::type_index <Branch> ():
		synthesized.insert(i);
		break;

	// Global variables are always instatiated,
	// even if not used by the unit
	case Atom::type_index <Qualifier> ():
	{
		auto &global = atom.as <Qualifier> ();
		synthesized.insert(i);
		synthesized.insert(global.underlying);
	} break;

	// Always synthesize returns
	case Atom::type_index <Returns> ():
		synthesized.insert(i);
		break;

	// Construction always takes place,
	// and its type should always be synthesized
	case Atom::type_index <Construct> ():
	{
		synthesized.insert(i);
		synthesized.insert(atom.as <Construct> ().type);
	} break;

	// If the type information indicates a nested struct,
	// then synthesize the nested struct regardless of usage
	case Atom::type_index <TypeInformation> ():
	{
		auto &type_field = atom.as <TypeInformation> ();
		if (type_field.down != -1)
			synthesized.insert(type_field.down);
	} break;

	// Call values should be evaluated only once by default;
	// at linking time, it may be possible to inline functions.
	case Atom::type_index <Call> ():
		synthesized.insert(i);
		break;

	default:
		break;
	}
}

} // namespace jvl::thunder
