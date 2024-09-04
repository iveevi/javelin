#include "ire/callable.hpp"
#include "ire/emitter.hpp"

namespace jvl::ire {

Callable::Callable() : Scratch()
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
	// Determine the set of used and synthesizable instructions
	std::unordered_set <thunder::index_t> used;
	std::unordered_set <thunder::index_t> synthesized;

	std::vector <thunder::Returns> returns;
	for (size_t i = 0; i < pool.size(); i++) {
		if (pool[i].is <thunder::Returns> ()) {
			returns.push_back(pool[i].as <thunder::Returns> ());
			detail::mark_used(pool, used, synthesized, i, true);
		}

		if (pool[i].is <thunder::Branch> ()
			|| pool[i].is <thunder::While> ()
			|| pool[i].is <thunder::End> ())
			detail::mark_used(pool, used, synthesized, i, true);

		// TODO: check scopes around returns...
	}

	// TODO:demotion on synthesized elements

	// TODO: compaction and validation
	thunder::Kernel kernel(thunder::Kernel::eCallable);
	kernel.name = name;
	kernel.atoms.resize(pointer);
	std::memcpy(kernel.atoms.data(), pool.data(), sizeof(thunder::Atom) * pointer);
	kernel.synthesized = synthesized;
	kernel.used = used;

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
