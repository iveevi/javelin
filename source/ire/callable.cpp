#include "ire/callable.hpp"
#include "ire/emitter.hpp"

namespace jvl::ire {

Callable::Callable() : Buffer()
{
	static size_t id = 0;
	cid = id++;
	name = fmt::format("callable{}", cid);

	tracked()[cid] = this;
}

Callable::Callable(const Callable &other)
{
	*this = other;
}

// TODO: destructor will erase the entry if the active * == this

Callable &Callable::operator=(const Callable &other)
{
	if (this != &other) {
		pool = other.pool;
		pointer = other.pointer;
		cid = other.cid;
		name = other.name;

		// Replace with the most recent tracked version
		tracked()[cid] = this;
	}

	return *this;
}

thunder::Kernel Callable::export_to_kernel() const
{
	thunder::Kernel kernel(thunder::Kernel::eCallable);
	kernel.name = name;
	kernel.atoms.resize(pointer);
	std::memcpy(kernel.atoms.data(), pool.data(), sizeof(thunder::Atom) * pointer);
	return kernel;
}

void Callable::dump()
{
	fmt::println("------------------------------");
	fmt::println("CALLABLE ${} ({}/{})", name, pointer, pool.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++)
		fmt::println("   [{:4d}]: {}", i, pool[i].to_string());
}

} // namespace jvl::ire