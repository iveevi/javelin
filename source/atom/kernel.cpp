#include <map>

#include "atom/kernel.hpp"

namespace jvl::atom {

// Printing the IR stored in a kernel
// TODO: debug only?
void Kernel::dump()
{
	// TODO: more properties
	fmt::println("------------------------------");
	fmt::println("KERNEL:");
	fmt::println("    {} atoms", atoms.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < atoms.size(); i++) {
		if (synthesized.contains(i))
			fmt::print("S");
		else
			fmt::print(" ");

		if (used.contains(i))
			fmt::print("U");
		else
			fmt::print(" ");

		fmt::print(" [{:4d}]: ", i);
			atom::dump_ir_operation(atoms[i]);
		fmt::print("\n");
	}
}

// Generating GLSL source code
std::string Kernel::synthesize_glsl(const std::string &version_number)
{
	// Final generated source
	std::string source;

	// Version header
	source += "#version " + version_number + "\n";
	source += "\n";

	// Gather all necessary structs
	wrapped::hash_table <int, std::string> struct_names;

	int struct_index = 0;
	for (int i = 0; i < atoms.size(); i++) {
		if (!synthesized.contains(i))
			continue;

		atom::General g = atoms[i];
		if (!g.is <atom::TypeField>())
			continue;

		atom::TypeField tf = g.as <atom::TypeField>();
		if (tf.next == -1 && tf.down == -1)
			continue;

		// TODO: handle nested structs (down)

		std::string struct_name = fmt::format("s{}_t", struct_index++);
		struct_names[i] = struct_name;

		std::string struct_source;
		struct_source = fmt::format("struct {} {{\n", struct_name);

		int field_index = 0;
		int j = i;
		while (j != -1) {
			atom::General g = atoms[j];
			if (!g.is<atom::TypeField>())
				abort();

			atom::TypeField tf = g.as<atom::TypeField>();
			// TODO: nested
			// TODO: put this whole thing in a method

			struct_source += fmt::format(
				"    {} f{};\n", atom::type_table[tf.item], field_index++);

			j = tf.next;
		}

		struct_source += "};\n\n";

		source += struct_source;
	}

	// Gather all global shader variables
	std::map <int, std::string> lins;
	std::map <int, std::string> louts;
	std::string push_constant;

	for (int i = 0; i < atoms.size(); i++) {
		if (!used.count(i))
			continue;

		auto op = atoms[i];
		if (!op.is <atom::Global> ())
			continue;

		auto global = std::get <atom::Global> (op);
		auto type = atom::type_name(atoms.data(), struct_names, i, -1);
		if (global.qualifier == atom::Global::layout_in)
			lins[global.binding] = type;
		else if (global.qualifier == atom::Global::layout_out)
			louts[global.binding] = type;
		else if (global.qualifier == atom::Global::push_constant)
			push_constant = type;
	}

	// Global shader variables
	for (const auto &[binding, type] : lins) {
		source += fmt::format("layout (location = {}) in {} _lin{};\n",
				binding, type, binding);
	}

	if (lins.size())
		source += "\n";

	for (const auto &[binding, type] : louts)
		source += fmt::format("layout (location = {}) out {} _lout{};\n",
				binding, type, binding);

	if (louts.size())
		source += "\n";

	// TODO: check for vulkan or opengl, etc
	if (push_constant.size()) {
		source += "layout (push_constant) uniform constants\n";
		source += "{\n";
		source += "     " + push_constant + " _pc;\n";
		source += "}\n";
		source += "\n";
	}

	// Main function
	source += "void main()\n";
	source += "{\n";
	source += atom::synthesize_glsl_body(atoms.data(), struct_names, synthesized, atoms.size());
	source += "}\n";

	return source;
}

} // namespace jvl::atom
