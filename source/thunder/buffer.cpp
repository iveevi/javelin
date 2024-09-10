#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/kernel.hpp"
#include "thunder/buffer.hpp"
#include "logging.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

MODULE(buffer);

Buffer::Buffer() : pointer(0), pool(4) {}

index_t Buffer::emit(const Atom &atom, bool enable_classification)
{
	if (pointer >= pool.size())
		pool.resize((pool.size() << 1));

	JVL_ASSERT(pointer < pool.size(),
		"scratch buffer pointer is out of bounds ({} >= {})",
		pointer, pool.size());

	pool[pointer] = atom;
	if (enable_classification) {
		if (auto td = classify(pointer))
			types[pointer] = td;
	}
	
	return pointer++;
}

// Kernel transfer
Kernel Buffer::export_to_kernel() const
{
	// TODO: export the kernel, then optimize and dce, and then validate...
	// validate();

	// TODO: run through optimizations

	thunder::Kernel kernel(thunder::Kernel::eAll);
	kernel.atoms = pool;
	kernel.atoms.resize(pointer);

	return kernel;
}

// Validation for kernels
void Buffer::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp

	wrapped::hash_table <index_t, index_t> inputs;
	wrapped::hash_table <index_t, index_t> outputs;

	for (int i = 0; i < pointer; i++) {
		thunder::Atom g = pool[i];
		if (!g.is <thunder::Global>())
			continue;

		thunder::Global global = g.as <thunder::Global> ();
		if (global.qualifier == thunder::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == thunder::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

void Buffer::clear()
{
	pointer = 0;
	pool.clear();
	pool.resize(4);
}

void Buffer::dump() const
{
	for (size_t i = 0; i < pointer; i++) {
		std::string type_string = types.get(i).value_or(TypeDecl()).to_string();
		fmt::println("   [{:4d}]: {:40} :: {}", i, pool[i].to_string(), type_string);
	}
}

// Utility methods
Atom &Buffer::operator[](size_t i)
{
	return pool[i];
}

const Atom &Buffer::operator[](size_t i) const
{
	return pool[i];
}

std::vector <index_t> Buffer::expand_list(index_t i) const
{
	std::vector <index_t> args;
	while (i != -1) {
		auto &atom = pool[i];
		assert(atom.is <List> ());

		List list = atom.as <List> ();
		args.push_back(list.item);

		i = list.next;
	}

	return args;
}

std::vector <TypeDecl> Buffer::expand_list_types(index_t i) const
{
	std::vector <TypeDecl> args;
	while (i != -1) {
		auto &atom = pool[i];
		assert(atom.is <List> ());

		List list = atom.as <List> ();

		if (types.contains(list.item))
			args.push_back(types.at(list.item));
		else
			args.push_back(TypeDecl());

		i = list.next;
	}

	return args;
}

} // namespace jvl::thunder
