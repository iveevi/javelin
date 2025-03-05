#include "thunder/buffer.hpp"
#include "thunder/properties.hpp"

namespace jvl::thunder {

Index Buffer::reference_of(Index i)
{
	auto &atom = atoms[i];

	switch (atom.index()) {
	variant_case(Atom, Swizzle):
		return reference_of(atom.as <Swizzle> ().src);
	variant_case(Atom, Load):
		return reference_of(atom.as <Load> ().src);
	variant_case(Atom, ArrayAccess):
		return reference_of(atom.as <ArrayAccess> ().src);
	default:
		return i;
	}
}

void Buffer::mark_children(Index i)
{
	auto &atom = atoms[i];

	switch (atom.index()) {

	// Store destinations should always be synthesized
	variant_case(Atom, Store):
	{
		auto &dst = atom.as <Store> ().dst;
		return mark(reference_of(dst), true);
	}
	
	// Qualifier's underlying type is required
	variant_case(Atom, Qualifier):
	{
		auto &underlying = atom.as <Qualifier> ().underlying;
		if (underlying != -1)
			mark(underlying, true);
	} break;

	// Constructor's type should always be synthesized
	variant_case(Atom, Construct):
	{
		auto &type = atom.as <Construct> ().type;
		return mark(type, true);
	}

	// Type field's nested types are required
	variant_case(Atom, TypeInformation):
	{
		auto &down = atom.as <TypeInformation> ().down;
		if (down != -1)
			mark(down, true);
	} break;

	// Call's return type should be available
	variant_case(Atom, Call):
	{
		auto &type = atom.as <Call> ().type;
		return mark(type, true);
	}

	// Array access' source is required
	variant_case(Atom, ArrayAccess):
	{
		auto src = atom.as <ArrayAccess> ().src;
		return mark(reference_of(src), true);
	}

	default:
		break;
	}
}

bool Buffer::naturally_forced(const Atom &atom)
{
	switch (atom.index()) {

	variant_case(Atom, Qualifier):
	{
		auto &kind = atom.as <Qualifier> ().kind;
		if (kind == acceleration_structure)
			return true;
	} break;

	variant_case(Atom, Intrinsic):
	{
		auto &opn = atom.as <Intrinsic> ().opn;
		return side_effects(opn);
	}

	variant_case(Atom, Construct):
	{
		auto &idx = atom.as <Construct> ().type;
		auto &type = types[idx];
		
		if (type.is <BufferReferenceType> ())
			return true;
	} break;

	variant_case(Atom, Store):
	variant_case(Atom, Return):
	variant_case(Atom, Branch):
	variant_case(Atom, Call):
		return true;

	default:
		break;
	}

	return false;
}

bool children_required(const Atom &atom)
{
	switch (atom.index()) {
	variant_case(Atom, Construct):
		return true;
	default:
		break;
	}

	return false;
}

void Buffer::mark(Index i, bool forced)
{
	if (marked.contains(i))
		return;

	// TODO: also track instruction usage..., .e.g avoid duplicates
	// if feasible (e.g. avoid instatiating large structures)

	// Either by nature of the instruction or another instruction requires it
	forced |= naturally_forced(atoms[i]);

	if (forced) {
		marked.insert(i);
		mark_children(i);
	} else if (children_required(atoms[i])) {
		// Cases in which children are still required
		mark_children(i);
	}
}

} // namespace jvl::thunder
