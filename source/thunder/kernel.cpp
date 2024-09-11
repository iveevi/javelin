#include "thunder/atom.hpp"
#include "thunder/kernel.hpp"
#include "thunder/linkage.hpp"
#include "logging.hpp"

namespace jvl::thunder {

MODULE(kernel);

index_t generate_type_declaration(Linkage &linkage, const std::vector <Atom> &atoms, index_t index)
{
	auto &block = linkage.blocks[-1];

	if (block.struct_map.contains(index))
		return block.struct_map[index];

	Linkage::struct_declaration decl;

	index_t i = index;
	while (i != -1) {
		Atom g = atoms[i];
		if (!g.is <TypeInformation> ())
			abort();

		TypeInformation tf = g.as <TypeInformation> ();

		Linkage::struct_element element;
		if (tf.down != -1)
			element.nested = generate_type_declaration(linkage, atoms, tf.down);
		else
			element.item = tf.item;

		decl.push_back(element);

		i = tf.next;
	}

	return (block.struct_map[index] = linkage.insert(decl));
}

// Linkage model from the kernel
Linkage Kernel::linkage() const
{
	JVL_STAGE_SECTION("kernel-linkage");

	Linkage linkage;

	// TODO: preserve the cid if present
	linkage.blocks[-1] = Linkage::block_t { name, {}, {}, atoms };
	linkage.sorted = { -1 };

	// Generate struct information for linkage
	auto synthesized = detail::synthesize_list(atoms);

	// Generate block information
	auto &block = linkage.blocks[-1];

	for (int i = 0; i < atoms.size(); i++) {
		if (!synthesized.contains(i))
			continue;

		auto atom = atoms[i];

		if (auto call = atom.get <Call> ())
			linkage.callables.insert(call->cid);

		if (auto global = atom.get <Qualifier> ()) {
			index_t type = global->underlying;
			index_t generated_type = generate_type_declaration(linkage, atoms, type);
			index_t binding = global->numerical;

			// TODO: the kernel must undergo validation
			switch (global->kind) {
			case layout_in:
				linkage.lins[binding] = generated_type;
				break;
			case layout_out:
				linkage.louts[binding] = generated_type;
				break;
			case parameter:
				// Need the concrete type for parameters,
				// so that the type fields can be accessed from JIT
				// TODO: find some way to fix this
				block.parameters[binding] = type;
				break;
			case push_constant:
				linkage.push_constant = generated_type;
				break;
			default:
				break;
			}
		}

		if (auto returns = atom.get <Returns> ())
			block.returns = returns->type;

		if (auto type_field = atom.get <TypeInformation> ())
			generate_type_declaration(linkage, atoms, i);
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
