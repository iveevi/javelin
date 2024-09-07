#include "thunder/scratch.hpp"

namespace jvl::thunder {

Scratch::Scratch() : pointer(0), pool(4) {}

void Scratch::reserve(size_t size)
{
	pool.resize(std::max(pool.size(), pointer + size));
}

index_t Scratch::emit(const Atom &atom)
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
	std::memset(pool.data(), 0, pool.size() * sizeof(Atom));
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
