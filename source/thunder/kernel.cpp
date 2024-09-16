#include "logging.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/kernel.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"

namespace jvl::thunder {

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
	JVL_STAGE_SECTION(kernel-linkage);

	Linkage linkage;

	// TODO: preserve the cid if present
	linkage.blocks[-1] = Linkage::block_t(*this, name);
	linkage.sorted = { -1 };

	// Generate block information
	auto &block = linkage.blocks[-1];

	for (size_t i = 0; i < pointer; i++) {
		if (!synthesized.contains(i))
			continue;

		auto atom = atoms[i];

		if (auto call = atom.get <Call> ())
			linkage.callables.insert(call->cid);

		if (auto qualifier = atom.get <Qualifier> ()) {
			index_t type = qualifier->underlying;
			index_t generated_type = generate_type_declaration(linkage, atoms, type);
			index_t binding = qualifier->numerical;

			switch (qualifier->kind) {
			case layout_in_flat:
			case layout_in_noperspective:
			case layout_in_smooth:
				linkage.lins[binding] = { generated_type, qualifier->kind };
				break;
			case layout_out_flat:
			case layout_out_noperspective:
			case layout_out_smooth:
				linkage.louts[binding] = { generated_type, qualifier->kind };
				break;
			case isampler_1d:
			case isampler_2d:
			case isampler_3d:
			case usampler_1d:
			case usampler_2d:
			case usampler_3d:
			case sampler_1d:
			case sampler_2d:
			case sampler_3d:
				linkage.samplers[binding] = qualifier->kind;
				break;
			case parameter:
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

	if (compatible_with(eVertexShader))
		fmt::println(".... vertex shader");

	if (compatible_with(eFragmentShader))
		fmt::println(".... fragment shader");

	if (compatible_with(eCallable))
		fmt::println(".... callable");

	fmt::println("------------------------------");

	Buffer::dump();
}

// Generating GLSL source code
std::string Kernel::compile(const profiles::glsl_version &version)
{
	return linkage().resolve().generate_glsl(version);
}

// Generating C++ source code
std::string Kernel::compile(const profiles::cpp_standard &)
{
	return linkage().resolve().generate_cplusplus();
}

} // namespace jvl::atom
