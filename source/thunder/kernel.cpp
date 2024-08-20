#include "thunder/atom.hpp"
#include "thunder/kernel.hpp"
#include "thunder/linkage.hpp"

namespace jvl::thunder {

// Linkage model from the kernel
Linkage Kernel::linkage() const
{
	Linkage linkage;

	// TODO: preserve the cid if present
	linkage.blocks[-1] = Linkage::block_t { {}, synthesized, {}, atoms };
	linkage.sorted = { -1 };

	// Generate struct information for linkage
	std::function <index_t (index_t)> generate_type_declaration;
	generate_type_declaration = [&](index_t index) -> index_t {
		Linkage::struct_declaration decl;

		index_t i = index;
		while (i != -1) {
			Atom g = atoms[i];
			if (!g.is <TypeField> ())
				abort();

			TypeField tf = g.as <TypeField> ();

			Linkage::struct_element element;
			if (tf.down != -1)
				element.nested = generate_type_declaration(tf.down);
			else
				element.item = tf.item;

			decl.push_back(element);

			i = tf.next;
		}

		index_t s = linkage.insert(decl);
		linkage.blocks[-1].struct_map[index] = s;
		return s;
	};

	// Go through all USED instructions
	for (int i = 0; i < atoms.size(); i++) {
		if (!used.count(i))
			continue;

		auto op = atoms[i];
		if (auto call = op.get <Call> ()) {
			linkage.callables.insert(call->cid);
		} else if (auto global = op.get <Global> ()) {
			index_t type = generate_type_declaration(global->type);
			index_t binding = global->binding;

			// TODO: the kernel must undergo validation
			switch (global->qualifier) {
			case Global::layout_in:
				linkage.lins[binding] = type;
				break;
			case Global::layout_out:
				linkage.louts[binding] = type;
				break;
			case Global::parameter:
				linkage.blocks[-1].parameters[binding] = type;
				break;
			case Global::push_constant:
				linkage.push_constant = type;
				break;
			default:
				break;
			}
		} else if (auto returns = op.get <Returns> ()) {
			linkage.blocks[-1].returns = generate_type_declaration(returns->type);
		}

		if (!synthesized.count(i))
			continue;

		if (auto tf = op.get <TypeField> ())
			linkage.blocks[-1].struct_map[i] = generate_type_declaration(i);
	}

	return linkage;
}

// Printing the IR stored in a kernel
void Kernel::dump() const
{
	fmt::println("------------------------------");
	fmt::println("KERNEL:");
	fmt::println("~~~~~~~");
	fmt::println("atoms: {}", atoms.size());
	fmt::println("flags:");

	if (is_compatible(eVertexShader))
		fmt::println(".... vertex shader");

	if (is_compatible(eFragmentShader))
		fmt::println(".... fragment shader");

	if (is_compatible(eCallable))
		fmt::println(".... callable");

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
			dump_ir_operation(atoms[i]);
		fmt::print("\n");
	}
}

// Generating GLSL source code
std::string Kernel::synthesize_glsl(const std::string &version)
{
	return linkage().resolve().synthesize_glsl(version);
}

// Generating C++ source code
std::string Kernel::synthesize_cplusplus()
{
	return linkage().resolve().synthesize_glsl("?");
}

} // namespace jvl::atom
