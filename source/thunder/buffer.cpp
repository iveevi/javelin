#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/kernel.hpp"
#include "thunder/qualified_type.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

MODULE(buffer);

Buffer::Buffer() : types(4), atoms(4), pointer(0) {}

index_t Buffer::emit(const Atom &atom, bool enable_classification)
{
	if (pointer >= atoms.size()) {
		atoms.resize((atoms.size() << 1));
		types.resize(atoms.size());
	}

	JVL_ASSERT(pointer < atoms.size(),
		"scratch buffer pointer is out of bounds ({} >= {})",
		pointer, atoms.size());

	atoms[pointer] = atom;
	if (enable_classification) {
		types[pointer] = classify(pointer);
		include(pointer);
	}
	
	return pointer++;
}

// Kernel transfer
Kernel Buffer::export_to_kernel() const
{
	// TODO: export the kernel, then optimize and dce, and then validate...
	// validate();

	// TODO: run through optimizations
	// TODO: find the right kernel flags

	auto kernel = Kernel(*this, thunder::Kernel::eAll);
	
	kernel.atoms.resize(pointer);
	kernel.atoms.shrink_to_fit();
	
	kernel.types.resize(pointer);
	kernel.types.shrink_to_fit();

	return kernel;
}

// Validation for kernels
void Buffer::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp

	wrapped::hash_table <index_t, index_t> inputs;
	wrapped::hash_table <index_t, index_t> outputs;

	// for (int i = 0; i < pointer; i++) {
	// 	Atom g = atoms[i];
	// 	if (!g.is <Qualifier>())
	// 		continue;

	// 	Qualifier global = g.as <Qualifier> ();
	// 	if (global.kind == layout_in) {
	// 		int type = global.underlying;
	// 		int binding = global.numerical;
	// 		if (inputs.count(binding) && inputs[binding] != type)
	// 			fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
	// 		else
	// 			inputs[binding] = type;
	// 	} else if (global.kind == layout_out) {
	// 		int type = global.underlying;
	// 		int binding = global.numerical;
	// 		if (outputs.count(binding) && outputs[binding] != type)
	// 			fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
	// 		else
	// 			outputs[binding] = type;
	// 	}
	// }
}

void Buffer::clear()
{
	pointer = 0;
	atoms.clear();
	atoms.resize(4);
}

void Buffer::dump() const
{
	for (size_t i = 0; i < pointer; i++)
		fmt::println("   [{:4d}] {:45} :: {}", i, atoms[i], types[i]);
}

// Utility methods
Atom &Buffer::operator[](size_t i)
{
	return atoms[i];
}

const Atom &Buffer::operator[](size_t i) const
{
	return atoms[i];
}

std::vector <index_t> Buffer::expand_list(index_t i) const
{
	std::vector <index_t> args;
	while (i != -1) {
		auto &atom = atoms[i];
		JVL_ASSERT_PLAIN(atom.is <List> ());

		List list = atom.as <List> ();
		args.push_back(list.item);

		i = list.next;
	}

	return args;
}

std::vector <QualifiedType> Buffer::expand_list_types(index_t i) const
{
	std::vector <QualifiedType> args;
	while (i != -1) {
		auto &atom = atoms[i];
		JVL_ASSERT_PLAIN(atom.is <List> ());

		List list = atom.as <List> ();
		i = list.next;
		
		args.push_back(types[list.item]);
	}

	return args;
}

} // namespace jvl::thunder
