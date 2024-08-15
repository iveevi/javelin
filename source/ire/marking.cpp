#include "ire/emitter.hpp"

namespace jvl::ire::detail {

void mark_used(const std::vector <atom::General> &pool,
	       std::unordered_set <atom::index_t> &used,
	       std::unordered_set <atom::index_t> &synthesized,
	       int index, bool syn)
{
	if (index == -1)
		return;

	used.insert(index);

	atom::General g = pool[index];

	// TODO: std visit with methods
	if (auto ctor = g.get <atom::Construct> ()) {
		mark_used(pool, used, synthesized, ctor->type, true);
		mark_used(pool, used, synthesized, ctor->args, true);
	} else if (auto call = g.get <atom::Call> ()) {
		mark_used(pool, used, synthesized, call->ret, true);
		mark_used(pool, used, synthesized, call->args, true);
	} else if (auto list = g.get <atom::List> ()) {
		// TODO: get size for nodes...
		mark_used(pool, used, synthesized, list->item, true);
		mark_used(pool, used, synthesized, list->next, false);
		syn = false;
	} else if (auto load = g.get <atom::Load> ()) {
		mark_used(pool, used, synthesized, load->src, true);
	} else if (auto store = g.get <atom::Store> ()) {
		mark_used(pool, used, synthesized, store->src, false);
		mark_used(pool, used, synthesized, store->dst, false);
		syn = false;
	} else if (auto global = g.get <atom::Global> ()) {
		mark_used(pool, used, synthesized, global->type, true);
		syn = false;
	} else if (auto tf = g.get <atom::TypeField> ()) {
		mark_used(pool, used, synthesized, tf->down, false);
		mark_used(pool, used, synthesized, tf->next, false);
	} else if (auto op = g.get <atom::Operation> ()) {
		mark_used(pool, used, synthesized, op->args, false);
	} else if (auto intr = g.get <atom::Intrinsic> ()) {
		mark_used(pool, used, synthesized, intr->args, false);
		mark_used(pool, used, synthesized, intr->ret, false);
	} else if (auto ret = g.get <atom::Returns> ()) {
		mark_used(pool, used, synthesized, ret->args, true);
	} else if (auto cond = g.get <atom::Cond> ()) {
		mark_used(pool, used, synthesized, cond->cond, true);
	}

	if (syn)
		synthesized.insert(index);
}

} // namespace jvl::ire::detail
