#include "thunder/atom.hpp"

namespace jvl::thunder {

bool Atom::operator==(const Atom &B) const
{
	if (index() != B.index())
		return false;

	switch (index()) {

	case Atom::type_index <Global> ():
		return as <Global> () == B.as <Global> ();
		
	case Atom::type_index <TypeField> ():
		return as <TypeField> () == B.as <TypeField> ();
	
	case Atom::type_index <Primitive> ():
		return as <Primitive> () == B.as <Primitive> ();

	case Atom::type_index <Swizzle> ():
		return as <Swizzle> () == B.as <Swizzle> ();

	case Atom::type_index <Operation> ():
		return as <Operation> () == B.as <Operation> ();
	
	case Atom::type_index <Construct> ():
		return as <Construct> () == B.as <Construct> ();
	
	case Atom::type_index <Call> ():
		return as <Call> () == B.as <Call> ();
	
	case Atom::type_index <Intrinsic> ():
		return as <Intrinsic> () == B.as <Intrinsic> ();
	
	case Atom::type_index <List> ():
		return as <List> () == B.as <List> ();
	
	case Atom::type_index <Store> ():
		return as <Store> () == B.as <Store> ();
	
	case Atom::type_index <Load> ():
		return as <Load> () == B.as <Load> ();
	
	case Atom::type_index <Branch> ():
		return as <Branch> () == B.as <Branch> ();
	
	case Atom::type_index <While> ():
		return as <While> () == B.as <While> ();
	
	case Atom::type_index <Returns> ():
		return as <Returns> () == B.as <Returns> ();
	
	case Atom::type_index <End> ():
		return true;

	default:
	{
		fmt::println("unsupported Atom sub-type in operator==: {}", index());
		abort();
	}

	}

	return false;
}

} // namespace jvl::thunder