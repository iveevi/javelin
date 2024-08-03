#include "ire/op.hpp"

namespace jvl::ire::op {

// Non-reindexable
void reindex_vd::operator()(Primitive &) {}
void reindex_vd::operator()(Swizzle &) {}
void reindex_vd::operator()(End &) {}

// Useful
void reindex_vd::operator()(Global &g) { g.type = reindex[g.type]; }

void reindex_vd::operator()(TypeField &tf)
{
	tf.down = reindex[tf.down];
	tf.next = reindex[tf.next];
}

void reindex_vd::operator()(Cmp &cmp)
{
	cmp.a = reindex[cmp.a];
	cmp.b = reindex[cmp.b];
}

void reindex_vd::operator()(List &list)
{
	list.item = reindex[list.item];
	list.next = reindex[list.next];
}

void reindex_vd::operator()(Construct &ctor)
{
	ctor.type = reindex[ctor.type];
	ctor.args = reindex[ctor.args];
}

void reindex_vd::operator()(Store &store)
{
	store.dst = reindex[store.dst];
	store.src = reindex[store.src];
}

void reindex_vd::operator()(Load &load) { load.src = reindex[load.src]; }

void reindex_vd::operator()(Cond &cond)
{
	cond.cond = reindex[cond.cond];
	cond.failto = reindex[cond.failto];
}

void reindex_vd::operator()(Elif &elif)
{
	elif.cond = reindex[elif.cond];
	elif.failto = reindex[elif.failto];
}

} // namespace jvl::ire::op
