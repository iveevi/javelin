#include "thunder/atom.hpp"

namespace jvl::thunder {

void reindex_ir_operation(const wrapped::reindex <index_t> &reindexer, Atom &g)
{
	switch (g.index()) {

	// Globals
	case Atom::type_index <Global> ():
	{
		reindexer(g.as <Global> ().type);
	} break;

	// Type fields
	case Atom::type_index <TypeField> ():
	{
		reindexer(g.as <TypeField> ().down);
		reindexer(g.as <TypeField> ().next);
	} break;

	// Operations
	case Atom::type_index <Operation> ():
	{
		reindexer(g.as <TypeField> ().down);
		reindexer(g.as <TypeField> ().next);
	} break;

	// Intrinsic
	case Atom::type_index <Intrinsic> ():
	{
		reindexer(g.as <Intrinsic> ().args);
		reindexer(g.as <Intrinsic> ().ret);
	} break;

	// Returns
	case Atom::type_index <Returns> ():
	{
		reindexer(g.as <Returns> ().args);
	} break;

	// List chains
	case Atom::type_index <List> ():
	{
		reindexer(g.as <List> ().item);
		reindexer(g.as <List> ().next);
	} break;

	// Constructors
	case Atom::type_index <Construct> ():
	{
		reindexer(g.as <Construct> ().type);
		reindexer(g.as <Construct> ().args);
	} break;

	// Callables
	case Atom::type_index <Call> ():
	{
		reindexer(g.as <Call> ().ret);
		reindexer(g.as <Call> ().args);
	} break;

	// Stores
	case Atom::type_index <Store> ():
	{
		reindexer(g.as <Store> ().dst);
		reindexer(g.as <Store> ().src);
	} break;

	// Loads
	case Atom::type_index <Load> ():
	{
		reindexer(g.as <Load> ().src);
	} break;

	// Swizzles
	case Atom::type_index <Swizzle> ():
	{
		reindexer(g.as <Swizzle> ().src);
	} break;

	// Conditions
	case Atom::type_index <Cond> ():
	{
		reindexer(g.as <Cond> ().cond);
		reindexer(g.as <Cond> ().failto);
	} break;

	// Posterior conditions
	case Atom::type_index <Elif> ():
	{
		reindexer(g.as <Elif> ().cond);
		reindexer(g.as <Elif> ().failto);
	} break;

	// Loops
	case Atom::type_index <While> ():
	{
		reindexer(g.as <While> ().cond);
		reindexer(g.as <While> ().failto);
	} break;

	// Exception
	default:
	{
		fmt::println("unexpected atom encountered during reindexing");
		abort();
	} break;

	}
}

} // namespace jvl::ire::atom
