#include "thunder/kernel.hpp"
#include "thunder/scratch.hpp"
#include "logging.hpp"

namespace jvl::thunder {

MODULE(scratch);

Scratch::Scratch() : pointer(0), pool(4) {}

index_t Scratch::emit(const Atom &atom)
{
	if (pointer >= pool.size())
		pool.resize((pool.size() << 1));

	JVL_ASSERT(pointer < pool.size(),
		"scratch buffer pointer is out of bounds ({} >= {})",
		pointer, pool.size());

	pool[pointer] = atom;
	return pointer++;
}

// Kernel transfer
Kernel Scratch::export_to_kernel() const
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
void Scratch::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp
	// TODO: put into the scratch

	wrapped::hash_table <int, int> inputs;
	wrapped::hash_table <int, int> outputs;

	for (int i = 0; i < pointer; i++) {
		thunder::Atom g = pool[i];
		if (!g.is <thunder::Global>())
			continue;

		thunder::Global global = g.as <thunder::Global> ();
		if (global.qualifier == thunder::GlobalQualifier::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == thunder::GlobalQualifier::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

void Scratch::clear()
{
	pointer = 0;
	pool.clear();
	pool.resize(4);
}

void Scratch::dump()
{
	fmt::println("------------------------------");
	fmt::println("SCRATCH ({}/{})", pointer, pool.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++)
		fmt::println("   [{:4d}]: {}", i, pool[i].to_string());
}

} // namespace jvl::thunder
