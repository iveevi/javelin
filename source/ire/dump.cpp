#include "ire/atom.hpp"

namespace jvl::ire::atom {

void dump_ir_operation(const General &g)
{
	if (auto global = g.get <Global> ()) {
		static const char *qualifier_table[] = {
			"layout input",
			"layout output",
			"push_constant",
			"glsl:vertex:gl_Position"
		};

		fmt::print("global: %{} = ({}, {})",
				global->type,
				qualifier_table[global->qualifier],
				global->binding);
	} else if (auto tf = g.get <TypeField> ()) {
		fmt::print("type: ");
		if (tf->item != bad)
			fmt::print("{}", type_table[tf->item]);
		else if (tf->down != -1)
			fmt::print("{}", tf->down);
		else
			fmt::print("<BAD>");

		fmt::print(" -> ");
		if (tf->next >= 0)
			fmt::print("{}", tf->next);
		else
			fmt::print("(nil)");
	} else if (auto p = g.get <Primitive> ()) {
		fmt::print("primitive: {} = ", type_table[p->type]);

		switch (p->type) {
		case i32:
			fmt::print("{}", p->idata[0]);
			break;
		case f32:
			fmt::print("{:.2f}", p->fdata[0]);
			break;
		case vec4:
			fmt::print("({:.2f}, {:.2f}, {:.2f}, {:.2f})",
				p->fdata[0], p->fdata[1],
				p->fdata[2], p->fdata[3]);
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
	} else if (auto store = g.get <Store> ()) {
		fmt::print("store %{} -> %{}", store->src, store->dst);
	} else if (auto load = g.get <Load> ()) {
		fmt::print("load %{} #{}", load->src, load->idx);
	} else if (auto swizzle = g.get <Swizzle> ()) {
		fmt::print("swizzle %{} #{}",
				swizzle->src,
				Swizzle::name[swizzle->type]);
	} else if (auto cmp = g.get <Cmp> ()) {
		const char *cmp_table[] = {"=", "!="};
		fmt::print("cmp %{} {} %{}", cmp->a, cmp_table[cmp->type], cmp->b);
	} else if (auto op = g.get <Operation> ()) {
		fmt::print("op ${} %{}", Operation::name[op->type], op->args);
	} else if (auto intr = g.get <Intrinsic> ()) {
		fmt::print("intr ${} %{} -> %{}", intr->name, intr->args, intr->ret);
	} else if (auto cond = g.get <Cond> ()) {
		fmt::print("cond %{} -> %{}", cond->cond, cond->failto);
	} else if (auto elif = g.get <Elif> ()) {
		if (elif->cond >= 0)
			fmt::print("elif %{} -> %{}", elif->cond, elif->failto);
		else
			fmt::print("elif (nil) -> %{}", elif->failto);
	} else if (auto loop = g.get <While> ()) {
		fmt::print("while %{} -> %{}", loop->cond, loop->failto);
	} else if (g.is <End> ()) {
		fmt::print("end");
	} else {
		fmt::print("<?>");
	}
}

} // namespace jvl::ire::atom
