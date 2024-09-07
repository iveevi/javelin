#include "thunder/atom.hpp"
#include "thunder/kernel.hpp"
#include "thunder/linkage.hpp"
#include "logging.hpp"

namespace jvl::thunder {

std::unordered_set <index_t> referenced_type_fields(const std::vector <Atom> &atoms)
{
	std::unordered_set <index_t> set;

	for (index_t i = 0; i < atoms.size(); i++) {
		auto &atom = atoms[i];

		if (auto global = atom.get <Global> ())
			set.insert(global->type);
		if (auto type_field = atom.get <TypeField> ())
			set.insert(type_field->down);
		if (auto operation = atom.get <Operation> ())
			set.insert(operation->type);
	}

	if (set.contains(-1))
		set.erase(-1);

	return set;
}

index_t generate_type_declaration(Linkage &linkage, const std::vector <Atom> &atoms, index_t index)
{
	Linkage::struct_declaration decl;

	index_t i = index;
	fmt::println("generating type declaration");
	while (i != -1) {
		Atom g = atoms[i];
		fmt::println("  > typefield: {}", g);
		if (!g.is <TypeField> ())
			abort();

		TypeField tf = g.as <TypeField> ();

		Linkage::struct_element element;
		if (tf.down != -1)
			element.nested = generate_type_declaration(linkage, atoms, tf.down);
		else
			element.item = tf.item;

		decl.push_back(element);

		i = tf.next;
	}

	index_t s = linkage.insert(decl);
	linkage.blocks[-1].struct_map[index] = s;
	return s;
}

// Linkage model from the kernel
Linkage Kernel::linkage() const
{
	Linkage linkage;

	// TODO: preserve the cid if present
	linkage.blocks[-1] = Linkage::block_t { name, {}, {}, atoms };
	linkage.sorted = { -1 };

	// Generate struct information for linkage
	for (auto i : referenced_type_fields(atoms))
		generate_type_declaration(linkage, atoms, i);

	// Generate block information
	auto &block = linkage.blocks[-1];

	for (int i = 0; i < atoms.size(); i++) {
		auto op = atoms[i];
		if (auto call = op.get <Call> ()) {
			linkage.callables.insert(call->cid);
		} else if (auto global = op.get <Global> ()) {
			index_t type = block.struct_map.at(global->type);
			index_t binding = global->binding;

			// TODO: the kernel must undergo validation
			switch (global->qualifier) {
			case GlobalQualifier::layout_in:
				linkage.lins[binding] = type;
				break;
			case GlobalQualifier::layout_out:
				linkage.louts[binding] = type;
				break;
			case GlobalQualifier::parameter:
				block.parameters[binding] = type;
				break;
			case GlobalQualifier::push_constant:
				linkage.push_constant = type;
				break;
			default:
				break;
			}
		} else if (auto returns = op.get <Returns> ()) {
			block.returns = returns->type;
		}
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
	for (size_t i = 0; i < atoms.size(); i++)
		fmt::println("  [{:4d}]: {}", i, atoms[i].to_string());
}

// Generating GLSL source code
std::string Kernel::generate_glsl(const std::string &version)
{
	return linkage().resolve().generate_glsl(version);
}

// Generating C++ source code
std::string Kernel::generate_cplusplus()
{
	return linkage().resolve().generate_cplusplus();
}

} // namespace jvl::atom
