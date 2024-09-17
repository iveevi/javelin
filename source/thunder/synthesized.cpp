#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/linkage.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(include-atoms);

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
		synthesized.insert(i);
		break;

	case Atom::type_index <Construct> ():
	{
		auto &constructor = atom.as <Construct> ();
		if (constructor.transient)
			synthesized.insert(i);

		JVL_ASSERT(constructor.type < (index_t) pointer,
			"construct type is out of bounds: {} (pointer = {})",
			atom, pointer);

		QualifiedType qt = types[constructor.type];
		if (qt.is <ArrayType> ())
			synthesized.insert(i);
	} break;

	case Atom::type_index <TypeInformation> ():
	{
		auto &type_field = atom.as <TypeInformation> ();
		if (type_field.down != -1)
			synthesized.insert(type_field.down);
	} break;

	}
}

} // namespace jvl::thunder
