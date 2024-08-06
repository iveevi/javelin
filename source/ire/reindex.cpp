#include "ire/op.hpp"

namespace jvl::ire::op {

// Non-reindexable
void reindex_vd::operator()(Primitive &) {}
void reindex_vd::operator()(Swizzle &) {}
void reindex_vd::operator()(End &) {}

// Useful
void reindex_vd::operator()(Global &g)
{
	g.type = reindexer[g.type];
}

void reindex_vd::operator()(TypeField &tf)
{
	tf.down = reindexer[tf.down];
	tf.next = reindexer[tf.next];
}

void reindex_vd::operator()(Cmp &cmp)
{
	cmp.a = reindexer[cmp.a];
	cmp.b = reindexer[cmp.b];
}

void reindex_vd::operator()(List &list)
{
	list.item = reindexer[list.item];
	list.next = reindexer[list.next];
}

void reindex_vd::operator()(Construct &ctor)
{
	ctor.type = reindexer[ctor.type];
	ctor.args = reindexer[ctor.args];
}

void reindex_vd::operator()(Store &store)
{
	store.dst = reindexer[store.dst];
	store.src = reindexer[store.src];
}

void reindex_vd::operator()(Load &load)
{
	load.src = reindexer[load.src];
}

void reindex_vd::operator()(Cond &cond)
{
	cond.cond = reindexer[cond.cond];
	cond.failto = reindexer[cond.failto];
}

void reindex_vd::operator()(Elif &elif)
{
	elif.cond = reindexer[elif.cond];
	elif.failto = reindexer[elif.failto];
}

} // namespace jvl::ire::op
