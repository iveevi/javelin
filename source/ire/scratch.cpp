#include "ire/scratch.hpp"

namespace jvl::ire {

Scratch::Scratch() : pointer(0), pool(4) {}

void Scratch::reserve(size_t size)
{
	pool.resize(std::max(pool.size(), pointer + size));
}

Scratch::index_t Scratch::emit(const thunder::Atom &atom)
{
	if (pointer >= pool.size())
		pool.resize((pool.size() << 1));

	pool[pointer] = atom;
	return pointer++;
}

void Scratch::clear()
{
	// Preserves the pool memory
	pointer = 0;
	std::memset(pool.data(), 0, pool.size() * sizeof(thunder::Atom));
}

void Scratch::dump()
{
	fmt::println("------------------------------");
	fmt::println("SCRATCH ({}/{})", pointer, pool.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++) {
		fmt::print("   [{:4d}]: ", i);
			thunder::dump_ir_operation(pool[i]);
		fmt::print("\n");
	}
}

} // namespace jvl::ire