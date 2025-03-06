#include <map>

#include "thunder/mir/molecule.hpp"

namespace jvl::thunder::mir {

struct NameGenerator {
	std::map <uint32_t, std::map <uint32_t, uint32_t>> counters;

	template <typename T>
	uint32_t next(Index i) {
		uint32_t t = Molecule::type_index <T> ();

		auto &map = counters[t];
		if (map.contains(i))
			return map[i];

		uint32_t n = map.size();
		map[i] = n;
		return n;
	}

	template <typename T>
	std::string name(const T &, Index i) {
		static constexpr const char *prefixes[] = {
			"typ",
			"val",
			"opt",
			"inr",
			"ctr",
			"jal",
			"str",
			"ldr",
			"idx",
			"blk",
			"jmp",
			"phi",
			"ret",
			"ref",
			"fld",
			"qlk",
		};

		static constexpr size_t index = Molecule::type_index <T> ();

		return fmt::format("{}{}", prefixes[index], next <T> (i));
	}

	std::string operator()(const Ref <Molecule> &ref) {
		auto ftn = [&](auto &self) -> std::string {
			return fmt::format("%{}", name(self, ref.index));
		};

		return std::visit(ftn, *ref);
	}
};

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

std::string stringify(NameGenerator &namer, const Molecule &molecule)
{
	switch (molecule.index()) {

	variant_case(Molecule, Type):
	{
		auto &type = molecule.as <Type> ();
		std::string result;

		for (auto &field : type.fields)
			result += fmt::format("{} ", stringify(namer, field));

		if (type.qualifiers.count > 0)
			result += ": ";

		for (auto &qualifier : type.qualifiers)
			result += fmt::format("{} ", thunder::tbl_qualifier_kind[qualifier]);

		return result;
	} break;

	variant_case(Molecule, Primitive):
	{
		auto &p = molecule.as <Primitive> ();
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

		return "not implemented";
	} break;

	variant_case(Molecule, Operation):
	{
		auto &opn = molecule.as <Operation> ();

		if (opn.b) {
			return fmt::format("{} {} {}",
				thunder::tbl_operation_code[opn.code],
				namer(opn.a),
				namer(opn.b));
		} else {
			return fmt::format("{} {}",
				thunder::tbl_operation_code[opn.code],
				namer(opn.a));
		}
	} break;

	variant_case(Molecule, Intrinsic):
	{
		auto &intr = molecule.as <Intrinsic> ();

		std::string result;

		result += fmt::format("{} ", thunder::tbl_intrinsic_operation[intr.opn]);

		for (auto &arg : intr.args)
			result += fmt::format("{} ", namer(arg.index));

		return result;
	} break;

	variant_case(Molecule, Construct):
	{
		auto &ctor = molecule.as <Construct> ();

		std::string result;

		result += fmt::format("{} new ", namer(ctor.type.promote()));
		for (auto &arg : ctor.args)
			result += fmt::format("{} ", namer(arg));

		result += fmt::format(": {}", thunder::tbl_constructor_mode[ctor.mode]);

		return result;
	
	} break;
	
	variant_case(Molecule, Block):
	{
		auto &block = molecule.as <Block> ();

		std::string result;

		result += "{\n";
		for (auto &molecule : block.body) {
			if (value_molecule(*molecule)) {
				result += fmt::format("    {:>5} = {}\n",
					namer(molecule),
					stringify(namer, *molecule));
			} else {
				result += fmt::format("    {}\n", stringify(namer, *molecule));
			}
		}
		result += "}";

		return result;
	} break;

	variant_case(Molecule, Store):
	{
		auto &store = molecule.as <Store> ();
		return fmt::format("store {} {}", namer(store.dst), namer(store.src));
	} break;

	variant_case(Molecule, Load):
	{
		auto &load = molecule.as <Load> ();
		return fmt::format("load {} {}", namer(load.src), load.idx);
	} break;

	variant_case(Molecule, Indexing):
	{
		auto &indexing = molecule.as <Indexing> ();
		return fmt::format("index {} {}", namer(indexing.src), namer(indexing.idx));
	} break;

	variant_case(Molecule, Return):
	{
		auto &returns = molecule.as <Return> ();
		return fmt::format("return {}", namer(returns.value));
	} break;
	
	variant_case(Molecule, Field):
	{
		auto &field = molecule.as <Field> ();
		if (auto type = field.get <Ref <Type>> ()) {
			return stringify(namer, *type.value());
		} else {
			auto pt = field.as <thunder::PrimitiveType> ();
			return thunder::tbl_primitive_types[pt];
		}
	} break;

	default:
		break;
	}

	return "?";
}

// General formatting only for blocks
std::string format_as(const Block &block)
{
	NameGenerator generator;
	return stringify(generator, block);
}

} // namespace jvl::thunder::mir