#include "thunder/atom.hpp"

namespace jvl::thunder {

MODULE(atom-comparison);

bool Atom::operator==(const Atom &B) const
{
	if (index() != B.index())
		return false;

	switch (index()) {
	case type_index <Qualifier> ():
		return as <Qualifier> () == B.as <Qualifier> ();
	case type_index <TypeInformation> ():
		return as <TypeInformation> () == B.as <TypeInformation> ();
	case type_index <Primitive> ():
		return as <Primitive> () == B.as <Primitive> ();
	case type_index <Swizzle> ():
		return as <Swizzle> () == B.as <Swizzle> ();
	case type_index <Operation> ():
		return as <Operation> () == B.as <Operation> ();
	case type_index <Construct> ():
		return as <Construct> () == B.as <Construct> ();
	case type_index <Call> ():
		return as <Call> () == B.as <Call> ();
	case type_index <Intrinsic> ():
		return as <Intrinsic> () == B.as <Intrinsic> ();
	case type_index <List> ():
		return as <List> () == B.as <List> ();
	case type_index <Store> ():
		return as <Store> () == B.as <Store> ();
	case type_index <Load> ():
		return as <Load> () == B.as <Load> ();
	case type_index <Branch> ():
		return as <Branch> () == B.as <Branch> ();
	case type_index <Returns> ():
		return as <Returns> () == B.as <Returns> ();
	default:
		break;
	}
		
	JVL_ABORT("unsupported Atom sub-type in operator==: {}", index());
}

} // namespace jvl::thunder