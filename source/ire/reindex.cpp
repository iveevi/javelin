#include "ire/op.hpp"

namespace jvl::ire::op {

void reindex_ir_operation(const wrapped::reindex &reindexer, General &g)
{
	if (g.is <Global> ()) {
		reindexer(g.as <Global> ().type);
	} else if (g.is <TypeField> ()) {
		reindexer(g.as <TypeField> ().down);
		reindexer(g.as <TypeField> ().next);
	} else if (g.is <Cmp> ()) {
		reindexer(g.as <Cmp> ().a);
		reindexer(g.as <Cmp> ().b);
	} else if (g.is <List> ()) {
		reindexer(g.as <List> ().item);
		reindexer(g.as <List> ().next);
	} else if (g.is <Construct> ()) {
		reindexer(g.as <Construct> ().type);
		reindexer(g.as <Construct> ().args);
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

} // namespace jvl::ire::op
