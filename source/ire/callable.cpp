#include "ire/callable.hpp"

namespace jvl::ire {

Callable::Callable() : pointer(0)
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

int Callable::emit(const atom::General &op)
{
	if (pointer >= pool.size())
		pool.resize(1 + (pool.size() << 2));

	pool[pointer] = op;

	return pointer++;
}

atom::Kernel Callable::export_to_kernel()
{
	// Determine the set of used and synthesizable instructions
	std::unordered_set <atom::index_t> used;
	std::unordered_set <atom::index_t> synthesized;

	std::vector <atom::Returns> returns;
	for (size_t i = 0; i < pool.size(); i++) {
		if (pool[i].is <atom::Returns> ()) {
			returns.push_back(pool[i].as <atom::Returns> ());
			detail::mark_used(pool, used, synthesized, i, true);
		}

		if (pool[i].is <atom::Cond> ()
			|| pool[i].is <atom::Elif> ()
			|| pool[i].is <atom::While> ()
			|| pool[i].is <atom::End> ())
			detail::mark_used(pool, used, synthesized, i, true);

		// TODO: check scopes around returns...
	}

	// TODO:demotion on synthesized elements

	// TODO: compaction and validation
	atom::Kernel kernel(atom::Kernel::eCallable);
	kernel.name = name;
	kernel.atoms.resize(pointer);
	std::memcpy(kernel.atoms.data(), pool.data(), sizeof(atom::General) * pointer);
	kernel.synthesized = synthesized;
	kernel.used = used;

	return kernel;
}

void Callable::dump()
{
	fmt::println("------------------------------");
	fmt::println("CALLABLE ({}/{})", pointer, pool.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++) {
		fmt::print("   [{:4d}]: ", i);
			atom::dump_ir_operation(pool[i]);
		fmt::print("\n");
	}
}

} // namespace jvl::ire
