#include "atom/atom.hpp"

namespace jvl::atom {

void reindex_ir_operation(const wrapped::reindex <index_t> &reindexer, General &g)
{
	if (g.is <Global> ()) {
		reindexer(g.as <Global> ().type);
	} else if (g.is <TypeField> ()) {
		reindexer(g.as <TypeField> ().down);
		reindexer(g.as <TypeField> ().next);
	} else if (g.is <Operation> ()) {
		reindexer(g.as <Operation> ().args);
		reindexer(g.as <Operation> ().ret);
	} else if (g.is <Intrinsic> ()) {
		reindexer(g.as <Intrinsic> ().args);
		reindexer(g.as <Intrinsic> ().ret);
	} else if (g.is <Returns> ()) {
		reindexer(g.as <Returns> ().args);
	} else if (g.is <List> ()) {
		reindexer(g.as <List> ().item);
		reindexer(g.as <List> ().next);
	} else if (g.is <Construct> ()) {
		reindexer(g.as <Construct> ().type);
		reindexer(g.as <Construct> ().args);
	} else if (g.is <Call> ()) {
		reindexer(g.as <Call> ().ret);
		reindexer(g.as <Call> ().args);
	} else if (g.is <Store> ()) {
		reindexer(g.as <Store> ().dst);
		reindexer(g.as <Store> ().src);
	} else if (g.is <Load> ()) {
		reindexer(g.as <Load> ().src);
	} else if (g.is <Swizzle> ()) {
		reindexer(g.as <Swizzle> ().src);
	} else if (g.is <Cond> ()) {
		reindexer(g.as <Cond> ().cond);
		reindexer(g.as <Cond> ().failto);
	} else if (g.is <Elif> ()) {
		reindexer(g.as <Elif> ().cond);
		reindexer(g.as <Elif> ().failto);
	} else if (g.is <While> ()) {
		reindexer(g.as <While> ().cond);
		reindexer(g.as <While> ().failto);
	}
}

} // namespace jvl::ire::atom
