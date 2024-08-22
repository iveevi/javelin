#include "thunder/atom.hpp"
#include "ire/callable.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::thunder {

void dump_ir_operation(const Atom &g)
{
	if (auto global = g.get <Global> ()) {
		fmt::print("global: %{} = ({}, {})",
			global->type,
			tbl_global_qualifier[global->qualifier],
			global->binding);
	} else if (auto tf = g.get <TypeField> ()) {
		fmt::print("type: ");
		if (tf->item != bad)
			fmt::print("{}", tbl_primitive_types[tf->item]);
		else if (tf->down != -1)
			fmt::print("%{}", tf->down);
		else
			fmt::print("<BAD>");

		fmt::print(" -> ");
		if (tf->next >= 0)
			fmt::print("%{}", tf->next);
		else
			fmt::print("(nil)");
	} else if (auto p = g.get <Primitive> ()) {
		fmt::print("primitive: {} = ", tbl_primitive_types[p->type]);

		switch (p->type) {
		case i32:
			fmt::print("{}", p->idata);
			break;
		case f32:
			fmt::print("{:.2f}", p->fdata);
			break;
		default:
			fmt::print("?");
			break;
		}
	} else if (auto list = g.get <List> ()) {
		fmt::print("list: %{} -> ", list->item);
		if (list->next >= 0)
			fmt::print("%{}", list->next);
		else
			fmt::print("(nil)");
	} else if (auto ctor = g.get <Construct> ()) {
		fmt::print("construct: %{} = ", ctor->type);
		if (ctor->args == -1)
			fmt::print("(nil)");
		else
			fmt::print("%{}", ctor->args);
	} else if (auto call = g.get <Call> ()) {
		ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
		fmt::print("call ${}:", cbl->name);
		if (call->args == -1)
			fmt::print(" (nil) -> ");
		else
			fmt::print(" %{} -> ", call->args);
		fmt::print("%{}", call->type);
	} else if (auto store = g.get <Store> ()) {
		fmt::print("store %{} -> %{}", store->src, store->dst);
	} else if (auto load = g.get <Load> ()) {
		fmt::print("load %{} #{}", load->src, load->idx);
	} else if (auto swizzle = g.get <Swizzle> ()) {
		fmt::print("swizzle %{} #{}",
				swizzle->src,
				tbl_swizzle_code[swizzle->code]);
	} else if (auto op = g.get <Operation> ()) {
		fmt::print("op ${} %{} -> %{}",
				tbl_operation_code[op->code],
				op->args,
				op->type);
	} else if (auto intr = g.get <Intrinsic> ()) {
		fmt::print("intr ${} %{} -> %{}",
				tbl_intrinsic_operation[intr->opn],
				intr->args,
				intr->type);
	} else if (auto branch = g.get <Branch> ()) {
		if (branch->cond >= 0)
			fmt::print("elif %{} -> %{}", branch->cond, branch->failto);
		else
			fmt::print("elif (nil) -> %{}", branch->failto);
	} else if (auto loop = g.get <While> ()) {
		fmt::print("while %{} -> %{}", loop->cond, loop->failto);
	} else if (auto ret = g.get <Returns> ()) {
		fmt::print("return %{} -> %{}", ret->args, ret->type);
	} else if (g.is <End> ()) {
		fmt::print("end");
	} else {
		fmt::print("<?>");
	}
}

} // namespace jvl::ire::thunder
