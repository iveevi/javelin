#include <fstream>

#include "thunder/buffer.hpp"
#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder {

MODULE(graphviz);

std::string instruction_string(const Atom &atom)
{
	static std::array <std::string, 14> labels {
		"Qualifier",
		"TypeInformation",
		"Primitive",
		"Swizzle",
		"Operation",
		"Intrinsic",
		"List",
		"Construct",
		"Call",
		"Store",
		"Load",
		"ArrayAccess",
		"Branch",
		"Returns",
	};

	return labels[atom.index()];
}

std::string instruction_color(const Atom &atom)
{
	static std::array <std::string, 14> colors = {
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
		"#A7C6ED"   // Powder Blue
	};

	return colors[atom.index()];
}

std::string instruction_arg0(const Atom &atom)
{
	static std::array <std::string, 14> colors = {
		"Underlying",
		"Down",
		"?",
		"Source",
		"First",
		"Operands",
		"Item",
		"Type",
		"Arguments",
		"Destination",
		"Source",
		"Source",
		"Condition",
		"Value",
	};

	return colors[atom.index()];
}

std::string instruction_arg1(const Atom &atom)
{
	static std::array <std::string, 14> colors = {
		"?",
		"Next",
		"?",
		"?",
		"Second",
		"?",
		"Next",
		"Arguments",
		"Type",
		"Source",
		"?",
		"Location",
		"Fails",
		"?",
	};

	return colors[atom.index()];
}

std::string instruction_record(const Atom &atom)
{
	switch (atom.index()) {

	variant_case(Atom, Qualifier):
	{
		auto &qualifier = atom.as <Qualifier> ();
		return fmt::format("{} | Numerical: {} | ",
			tbl_qualifier_kind[qualifier.kind],
			qualifier.numerical);
	}

	variant_case(Atom, Primitive):
	{
		auto &primitive = atom.as <Primitive> ();
		return primitive.value_string() + " | ";
	}

	variant_case(Atom, Swizzle):
	{
		auto &swizzle = atom.as <Swizzle> ();
		return tbl_swizzle_code[swizzle.code] + std::string(" | ");
	}

	variant_case(Atom, Operation):
	{
		auto &operation = atom.as <Operation> ();
		return tbl_operation_code[operation.code] + std::string(" | ");
	}

	variant_case(Atom, Intrinsic):
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		return tbl_intrinsic_operation[intrinsic.opn] + std::string(" | ");
	}

	variant_case(Atom, Construct):
	{
		auto &constructor = atom.as <Construct> ();
		return tbl_constructor_mode[constructor.mode] + std::string(" | ");
	}

	variant_case(Atom, Call):
	{
		auto &call = atom.as <Call> ();
		auto &buffer= TrackedBuffer::cache_load(call.cid);
		return buffer.name + " | ";
	}
	
	variant_case(Atom, Load):
	{
		auto &load = atom.as <Load> ();
		return fmt::format("Index: {} | ", load.idx);
	}
	
	variant_case(Atom, Branch):
	{
		auto &branch = atom.as <Branch> ();
		return tbl_branch_kind[branch.kind] + std::string(" | ");
	}

	default:
		break;
	}

	return "";
}

// TODO: option to configure fonts...
void Buffer::graphviz(const std::filesystem::path &path) const
{
	JVL_INFO("generating graphviz to \"{}\"", path.string());
	
	std::string result = "digraph G {\n";

	result += "\tnode [shape=record, style=\"filled, rounded\", fontname=\"IosevkaTerm Nerd Font Mono\"];\n";
	result += "\tedge [fontname=\"IosevkaTerm Nerd Font Mono\"];\n";

	// Labeling nodes
	for (size_t i = 0; i < pointer; i++) {
		auto &atom = atoms[i];
		auto &type = types[i];

		uint64_t hash = reinterpret_cast <const uint64_t &> (atom);
		uint32_t low = hash & 0xFFFFFFFF;
		uint32_t high = hash >> 32;
		
		std::string extra;
		if (!synthesized.contains(i))
			extra = ", style=\"filled, rounded, dashed\"";

		result += fmt::format("\tI{} [label=\"{{ {} [{}] | {}{} | {:08x}{:08x} }}\", fillcolor=\"{}\"{}]\n",
			i, instruction_string(atom),
			i, instruction_record(atom),
			type.to_string(),
			high, low,
			instruction_color(atom), extra);
	}

	result += "\n";

	// Connections
	for (size_t i = 0; i < pointer; i++) {
		auto addr = const_cast <Atom &> (atoms[i]).addresses();
		if (addr.a0 != -1) {
			result += fmt::format("\tI{} -> I{} [label=<<i>{}</i>>];\n",
				i, addr.a0, instruction_arg0(atoms[i]));
		}

		if (addr.a1 != -1) {
			result += fmt::format("\tI{} -> I{} [label=<<i>{}</i>>];\n",
				i, addr.a1, instruction_arg1(atoms[i]));
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

} // namespace jvl::thunder