// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

#include "common/io.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(mir);

namespace jvl::thunder::mir {

// TODO: subgraph for block...
static constexpr size_t NMOLES = Molecule::type_index <QualifierKind> () + 1;

std::string mole_name(const Molecule &mole)
{
	static std::array <const char *const, NMOLES> labels {
		// Primary instructions
		"Type",
		"Primitive",
		"Operation",
		"Intrinsic",
		"Construct",
		"Call",
		"Store",
		"Load",
		"Indexing",
		"Block",
		"Branch",
		"Phi",
		"Return",

		// Misecellaneous; for memory arena
		"Reference",
		"Field",
		"QualifierKind",
	};

	return labels[mole.index()];
}

std::string mole_color(const Molecule &mole)
{
	static std::array <std::string, NMOLES> colors = {
		"#FFB3BA",  // Pastel Pink
		"#FFDFBA",  // Peach
		"#FFFFBA",  // Pastel Yellow
		"#BAFFC9",  // Mint Green
		"#BAE1FF",  // Light Aqua
		"#D6BAFF",  // Lavender
		"#FFBAF0",  // Pastel Purple
		"#FF9B9B",  // Light Coral
		"#A0D3E8",  // Pastel Blue
		"#EAB8E4",  // Soft Lilac
		"#C8E6B9",  // Pale Green
		"#FFEDB2",  // Light Peach
		"#B2E1FF",  // Baby Blue
		"#A7C6ED",  // Powder Blue
		"#D4A5A5",  // Warm Blush
		"#C2C9A7"   // Soft Olive
	};

	return colors[mole.index()];
}

auto properties(const Molecule &mole)
{
	std::set <std::string> result;

	switch (mole.index()) {

	variant_case(Molecule, Type):
	{
		auto &type = mole.as <Type> ();

		for (auto ql : type.qualifiers)
			result.insert(tbl_qualifier_kind[ql]);
	} break;

	variant_case(Molecule, Primitive):
	{
		auto &prim = mole.as <Primitive> ();

		switch (prim.index()) {
		variant_case(Primitive, Int):
			result.insert(fmt::format("i32: {}", prim.as <Int> ()));
			break;
		variant_case(Primitive, Float):
			result.insert(fmt::format("f32: {}", prim.as <Float> ()));
			break;
		default:
			result.insert("?");
			break;
		}
	} break;

	variant_case(Molecule, Operation):
	{
		auto &opn = mole.as <Operation> ();
		result.insert(tbl_operation_code[opn.code]);
	} break;
	
	variant_case(Molecule, Intrinsic):
	{
		auto &intr = mole.as <Intrinsic> ();
		result.insert(tbl_intrinsic_operation[intr.code]);
	} break;

	default:
		break;
	}

	return result;
}

auto addresses(const Molecule &mole)
{
	std::set <std::pair <std::string, Index::index_type>> result;

	switch (mole.index()) {

	variant_case(Molecule, Construct):
	{
		auto &ctor = mole.as <Construct> ();

		result.insert(std::make_pair("Type", ctor.type.idx()));

		size_t i = 0;
		for (auto arg : ctor.args) {
			std::string name = fmt::format("Arg {}", i++);
			result.insert(std::make_pair(name, arg.idx()));
		}
	} break;
	
	variant_case(Molecule, Operation):
	{
		auto &opn = mole.as <Operation> ();

		if (opn.a)
			result.insert(std::make_pair("First", opn.a.idx()));
		if (opn.b)
			result.insert(std::make_pair("First", opn.b.idx()));
	} break;
	
	variant_case(Molecule, Intrinsic):
	{
		auto &intr = mole.as <Intrinsic> ();

		size_t i = 0;
		for (auto arg : intr.args) {
			std::string name = fmt::format("Arg {}", i++);
			result.insert(std::make_pair(name, arg.idx()));
		}
	} break;

	variant_case(Molecule, Store):
	{
		auto &store = mole.as <Store> ();
		result.insert(std::make_pair("Destination", store.dst.idx()));
		result.insert(std::make_pair("Source", store.src.idx()));
	} break;
	
	variant_case(Molecule, Load):
	{
		auto &load = mole.as <Load> ();
		result.insert(std::make_pair("Source", load.src.idx()));
	} break;
	
	variant_case(Molecule, Return):
	{
		auto &returns = mole.as <Return> ();
		result.insert(std::make_pair("Value", returns.value.idx()));
	} break;

	default:
		break;
	}

	return result;
}

void Block::graphviz(const std::filesystem::path &path) const
{
	JVL_INFO("generating graphviz (MIR) to \"{}\"", std::filesystem::weakly_canonical(path).string());
	
	std::string result = "digraph G {\n";

	result += "\tnode [shape=record, style=\"filled, rounded\", fontname=\"IosevkaTerm Nerd Font Mono\"];\n";
	result += "\tedge [fontname=\"IosevkaTerm Nerd Font Mono\"];\n";

	for (auto mole : body) {
		auto propset = properties(*mole);

		std::string propstr;
		for (auto s : propset)
			propstr += " | " + s;

		result += fmt::format("\tI{} [label=\"{{ {} [{}] {} }}\", fillcolor=\"{}\"]\n",
			mole.idx(),
			mole_name(*mole),
			mole.idx(),
			propstr,
			mole_color(*mole));
	}

	for (auto mole : body) {
		auto addrs = addresses(*mole);
		for (auto [l, i] : addrs) {
			result += fmt::format("\tI{} -> I{} [label=<<i>{}</i>>];\n",
				mole.idx(), i, l);
		}
	}
	
	result += "}";

	std::ofstream fout(path);
	if (!fout) {
		JVL_ERROR("failed to open file \"{}\"", path.string());
		return;
	}

	fout.write(result.c_str(), result.size());
	fout.close();
}

} // namespace jvl::thunder::mir

// TODO: dot generator...
// TODO: glsl function body generator...

auto ftn = procedure("area") << [](vec2 x, vec2 y)
{
	return abs(cross(x, y));
};

int main()
{
	ftn.display_assembly();
	link(ftn).write_assembly("ire.jvl.asm");

	// thunder::optimize_dead_code_elimination(ftn);
	thunder::optimize_deduplicate(ftn);
	ftn.display_assembly();

	auto mir = ftn.lower_to_mir();

	fmt::println("{}", mir);

	mir.graphviz("mir.dot");
}
