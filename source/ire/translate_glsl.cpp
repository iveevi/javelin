#include "ire/op.hpp"

namespace jvl::ire::op {

std::string translate_glsl_vd::operator()(const Cond &cond)
{
	next_indentation++;
	std::string c = defer(cond.cond, true);
	return "if (" + c + ") {";
}

std::string translate_glsl_vd::operator()(const Elif &elif)
{
	next_indentation = indentation--;
	if (elif.cond == -1)
		return "} else {";

	std::string c = defer(elif.cond, true);
	return "} else if (" + c + ") {";
}

std::string translate_glsl_vd::operator()(const End &)
{
	// TODO: verify which end?
	next_indentation = indentation--;
	return "}";
}

std::string translate_glsl_vd::operator()(const Global &global)
{
	switch (global.qualifier) {
	case Global::layout_out:
		return "_lout" + std::to_string(global.binding);
	case Global::layout_in:
		return "_lin" + std::to_string(global.binding);
	default:
		break;
	}

	return "<glo:?>";
}

std::string translate_glsl_vd::operator()(const Primitive &p)
{
	// TODO: fmt format
	switch (p.type) {
	case i32:
		return std::to_string(p.idata[0]);
	case f32:
		return std::to_string(p.fdata[0]);
	case vec4:
		return "vec4("
			+ std::to_string(p.fdata[0]) + ", "
			+ std::to_string(p.fdata[1]) + ", "
			+ std::to_string(p.fdata[2]) + ", "
			+ std::to_string(p.fdata[3])
			+ ")";
	default:
		break;
	}

	return "<prim:?>";
}

std::string translate_glsl_vd::operator()(const Load &load)
{
	// TODO: cache based on instruction address

	bool inl = inlining;
	fmt::println("load: inl={}", inl);
	std::string value = defer(load.src);
	if (inl)
		return value;

	typeof_vd typer(pool, struct_names);
	std::string type = typer.defer(load.src);
	std::string sym = "s" + std::to_string(generator++);
	return type + " " + sym + " = " + value + ";";
}

std::string translate_glsl_vd::operator()(const Store &store)
{
	std::string lhs = defer(store.dst);
	std::string rhs = defer(store.src);
	return lhs + " = " + rhs + ";";
}

std::string translate_glsl_vd::operator()(const Cmp &cmp)
{
	bool inl = inlining;
	std::string sa = defer(cmp.a, inl);
	std::string sb = defer(cmp.b, inl);
	switch (cmp.type) {
	case Cmp::eq:
		return sa + " == " + sb;
	default:
		break;
	}

	return "<cmp:?>";
}

std::string translate_glsl_vd::operator()(const Construct &ctor)
{
	bool inl = inlining;

	typeof_vd typer(pool, struct_names);
	std::string type = typer.defer(ctor.type);

	int args = ctor.args;
	std::string value = type + "(";
	while (args != -1) {
		op::General g = pool[args];
		if (!g.is <op::List> ())
			abort();

		op::List l = g.as <op::List> ();
		value += defer(l.item, true);

		if (l.next != -1)
			value += ", ";

		args = l.next;
	}

	value += ")";

	if (inl)
		return value;

	std::string sym = "s" + std::to_string(generator++);
	return type + " " + sym + " = " + value + ";";
}

std::string translate_glsl_vd::defer(int index, bool inl)
{
	inlining = inl;
	if (symbols.contains(index))
		return symbols[index];

	return std::visit(*this, pool[index]);
}

std::string translate_glsl_vd::eval(int index)
{
	std::string source = defer(index) + "\n";
	std::string space(indentation << 2, ' ');
	indentation = next_indentation;
	return space + source;
}

}
