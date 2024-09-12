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
		atoms = other.atoms;
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
	auto kernel = Buffer::export_to_kernel();
	kernel.name = name;
	return kernel;
}

void Callable::dump()
{
	fmt::println("------------------------------");
	fmt::println("CALLABLE ${} ({}/{})", name, pointer, atoms.size());
	fmt::println("------------------------------");
	Buffer::dump();
}

} // namespace jvl::ire