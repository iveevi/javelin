#include "thunder/scratch.hpp"
#include "logging.hpp"

namespace jvl::thunder {

MODULE(scratch);

Scratch::Scratch() : pointer(0), pool(4) {}

void Scratch::reserve(size_t size)
{
	pool.resize(std::max(pool.size(), pointer + size));
}

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
