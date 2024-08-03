#include "ire/op.hpp"

namespace jvl::ire::op {

void dump_vd::operator()(const Global &global)
{
	static const char *qualifier_table[] = {
		"layout input",
		"layout output"
	};

	printf("global: %%%d = (%s, %d)", global.type,
		qualifier_table[global.qualifier], global.binding);
}

void dump_vd::operator()(const TypeField &t)
{
	printf("type: ");
	if (t.item != bad)
		printf("%s", type_table[t.item]);
	else
		printf("%%%d", t.down);

	printf(" -> ");
	if (t.next >= 0)
		printf("%%%d", t.next);
	else
		printf("(nil)");
}

void dump_vd::operator()(const Primitive &p)
{
	printf("primitive: %s = ", type_table[p.type]);

	switch (p.type) {
	case i32:
		printf("%d", p.idata[0]);
		break;
	case f32:
		printf("%.2f", p.fdata[0]);
		break;
	case vec4:
		printf("(%.2f, %.2f, %.2f, %.2f)",
				p.fdata[0],
				p.fdata[1],
				p.fdata[2],
				p.fdata[3]);
		break;
	default:
		printf("?");
		break;
	}
}

void dump_vd::operator()(const List &list)
{
	printf("list: %%%d -> ", list.item);
	if (list.next >= 0)
		printf("%%%d", list.next);
	else
		printf("(nil)");
}

void dump_vd::operator()(const Construct &ctor)
{
	printf("construct: %%%d = ", ctor.type);
	if (ctor.args == -1)
		printf("(nil)");
	else
		printf("%%%d", ctor.args);
}

void dump_vd::operator()(const Store &store)
{
	printf("store %%%d -> %%%d", store.src, store.dst);
}

void dump_vd::operator()(const Load &load)
{
	printf("load %%%d", load.src);
}

void dump_vd::operator()(const Cmp &cmp)
{
	const char *cmp_table[] = { "=", "!=" };
	printf("cmp %%%d %s %%%d", cmp.a, cmp_table[cmp.type], cmp.b);
}

void dump_vd::operator()(const Cond &cond)
{
	printf("cond %%%d -> %%%d", cond.cond, cond.failto);
}

void dump_vd::operator()(const Elif &elif)
{
	if (elif.cond >= 0)
		printf("elif %%%d -> %%%d", elif.cond, elif.failto);
	else
		printf("elif (nil) -> %%%d", elif.failto);
}

void dump_vd::operator()(const End &)
{
	printf("end");
}

}
