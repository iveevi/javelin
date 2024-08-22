#include "ire/emitter.hpp"

namespace jvl::ire::detail {

void mark_used(const std::vector <thunder::Atom> &pool,
	       std::unordered_set <thunder::index_t> &used,
	       std::unordered_set <thunder::index_t> &synthesized,
	       int index, bool syn)
{
	if (index == -1)
		return;

	used.insert(index);

	thunder::Atom g = pool[index];

	// TODO: std visit with methods
	if (auto ctor = g.get <thunder::Construct> ()) {
		mark_used(pool, used, synthesized, ctor->type, true);
		mark_used(pool, used, synthesized, ctor->args, true);
	} else if (auto call = g.get <thunder::Call> ()) {
		mark_used(pool, used, synthesized, call->type, true);
		mark_used(pool, used, synthesized, call->args, true);
	} else if (auto list = g.get <thunder::List> ()) {
		// TODO: get size for nodes...
		mark_used(pool, used, synthesized, list->item, true);
		mark_used(pool, used, synthesized, list->next, false);
		syn = false;
	} else if (auto load = g.get <thunder::Load> ()) {
		mark_used(pool, used, synthesized, load->src, true);
	} else if (auto store = g.get <thunder::Store> ()) {
		mark_used(pool, used, synthesized, store->src, false);
		mark_used(pool, used, synthesized, store->dst, false);
		syn = false;
	} else if (auto global = g.get <thunder::Global> ()) {
		mark_used(pool, used, synthesized, global->type, true);
		syn = false;
	} else if (auto tf = g.get <thunder::TypeField> ()) {
		mark_used(pool, used, synthesized, tf->down, false);
		mark_used(pool, used, synthesized, tf->next, false);
	} else if (auto op = g.get <thunder::Operation> ()) {
		mark_used(pool, used, synthesized, op->args, false);
	} else if (auto intr = g.get <thunder::Intrinsic> ()) {
		mark_used(pool, used, synthesized, intr->args, false);
		mark_used(pool, used, synthesized, intr->type, false);
	} else if (auto ret = g.get <thunder::Returns> ()) {
		mark_used(pool, used, synthesized, ret->args, true);
	} else if (auto branch = g.get <thunder::Branch> ()) {
		mark_used(pool, used, synthesized, branch->cond, true);
	}

	if (syn)
		synthesized.insert(index);
}

} // namespace jvl::ire::detail
