#include "thunder/mir/molecule.hpp"

namespace jvl::thunder::mir {

bool value_molecule(const Molecule &molecule)
{
	switch (molecule.index()) {
	variant_case(Molecule, Return):
		return false;
	default:
		break;
	}

	return true;
}

template <typename T>
std::string format_as(const T &)
{
	return "molecule(?)";
}

std::string format_as(const Type &type)
{
	if (auto prim = type.get <PrimitiveType> ()) {
		return tbl_primitive_types[*prim];
	} else {
		auto &aggr = type.as <Aggregate> ();

		std::string result;

		for (auto &field : aggr.fields)
			result += fmt::format("%{} ", field.idx());

		if (type.qualifiers.count > 0)
			result += ": ";

		for (auto &qualifier : type.qualifiers)
			result += fmt::format("{} ", thunder::tbl_qualifier_kind[qualifier]);

		return result;
	}
}

std::string format_as(const Primitive &p)
{
	switch (p.index()) {
	variant_case(Primitive, Int):
		return fmt::format("i32 {}", p.as <Int> ());
	variant_case(Primitive, Float):
		return fmt::format("f32 {}", p.as <Float> ());
	variant_case(Primitive, Bool):
		return fmt::format("bool {}", p.as <Bool> ());
	variant_case(Primitive, String):
		return fmt::format("string \"{}\"", p.as <String> ());
	}

	return "primitive(?)";
}

std::string format_as(const Storage &storage)
{
	return fmt::format("storage %{}", storage.type.idx());
}

std::string format_as(const Operation &opn)
{
	if (opn.b) {
		return fmt::format("{} %{} %{}",
			thunder::tbl_operation_code[opn.code],
			opn.a.idx(), opn.b.idx());
	} else {
		return fmt::format("{} %{}",
			thunder::tbl_operation_code[opn.code],
			opn.a.idx());
	}
}

std::string format_as(const Intrinsic &intr)
{
	std::string result;

	result += fmt::format("{} ", thunder::tbl_intrinsic_operation[intr.code]);

	for (auto &arg : intr.args)
		result += fmt::format("%{} ", arg.idx());

	return result;
}

std::string format_as(const Construct &ctor)
{
	std::string result;

	result += fmt::format("%{} new ", ctor.type.idx());
	for (auto &arg : ctor.args)
		result += fmt::format("%{} ", arg.idx());

	result += fmt::format(": {}", thunder::tbl_constructor_mode[ctor.mode]);

	return result;
}

std::string format_as(const Block &block)
{
	std::string result;

	result += "{\n";
	for (auto &molecule : block.body) {
		if (value_molecule(*molecule)) {
			result += fmt::format("    %{} = {}\n",
				molecule.idx(), *molecule);
		} else {
			result += fmt::format("    {}\n", *molecule);
		}
	}
	result += "}";

	return result;
}

std::string format_as(const Store &store)
{
	return fmt::format("store %{} %{}", store.dst.idx(), store.src.idx());
}

std::string format_as(const Load &load)
{
	return fmt::format("load %{} {}", load.src.idx(), load.idx);
}

std::string format_as(const Indexing &indexing)
{
	return fmt::format("index %{} %{}", indexing.src.idx(), indexing.idx.idx());
}

std::string format_as(const Return &returns)
{
	return fmt::format("return %{}", returns.value.idx());
}

std::string format_as(const Molecule &mole)
{
	auto ftn = [](auto x) { return format_as(x); };
	return std::visit(ftn, mole);
}

} // namespace jvl::thunder::mir