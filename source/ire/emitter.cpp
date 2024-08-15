#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "atom/atom.hpp"
#include "ire/emitter.hpp"
#include "ire/tagged.hpp"
#include "wrapped_types.hpp"
#include "ire/callable.hpp"

namespace jvl::ire {

Emitter::Emitter() : pointer(0) {}

void Emitter::clear()
{
	pointer = 0;
	std::memset(pool.data(), 0, pool.size() * sizeof(atom::General));

	// Reset usages
	used.clear();
	synthesized.clear();
}

void Emitter::compact()
{
	std::vector <atom::General> dual(pool.size());
	wrapped::reindex <atom::index_t> reindexer;
	std::tie(pointer, reindexer) = detail::ir_compact_deduplicate(pool.data(), dual.data(), synthesized, pointer);
	std::memset(pool.data(), 0, pool.size() * sizeof(atom::General));
	std::memcpy(pool.data(), dual.data(), pointer * sizeof(atom::General));
	synthesized = reindexer(synthesized);
	used = reindexer(used);
}

// Dead code elimination
void Emitter::mark_used(int index, bool syn)
{
	// If we are in a scope, this is all determined later...
	if (scopes.size())
		return;

	detail::mark_used(pool, used, synthesized, index, syn);
}

// Emitting instructions during function invocation
int Emitter::emit(const atom::General &op)
{
	if (scopes.size())
		return scopes.top().get().emit(op);

	if (pointer >= pool.size())
		pool.resize(1 + (pool.size() << 2));

	pool[pointer] = op;

	return pointer++;
}

int Emitter::emit_main(const atom::General &op)
{
	int p = emit(op);
	synthesized.insert(p);
	mark_used(p, true);
	return p;
}

int Emitter::emit_main(const atom::Cond &cond)
{
	int p = emit_main((atom::General) cond);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::Elif &elif)
{
	int p = emit_main((atom::General) elif);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::While &loop)
{
	int p = emit_main((atom::General) loop);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::End &end)
{
	int p = emit_main((atom::General) end);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	return p;
}

void Emitter::control_flow_callback(int ref, int p)
{
	atom::General op;
	if (scopes.size())
		op = scopes.top().get().pool[ref];
	else
		op = pool[ref];

	if (op.is <atom::Cond> ()) {
		op.as <atom::Cond> ().failto = p;
	} else if (op.is <atom::Elif> ()) {
		op.as <atom::Elif> ().failto = p;
	} else if (op.is <atom::While> ()) {
		op.as <atom::While> ().failto = p;
	} else {
		fmt::print("op not conditional, is actually:");
			atom::dump_ir_operation(op);
		fmt::print("\n");

		abort();
	}
}

// Validating IR
void Emitter::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp

	wrapped::hash_table <int, int> inputs;
	wrapped::hash_table <int, int> outputs;

	for (int i = 0; i < pointer; i++) {
		if (!used.contains(i))
			continue;

		atom::General g = pool[i];
		if (!g.is <atom::Global>())
			continue;

		atom::Global global = g.as <atom::Global> ();
		if (global.qualifier == atom::Global::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == atom::Global::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

// Kernel transfer
atom::Kernel Emitter::export_to_kernel()
{
	// Compaction, validation, ...
	compact();
	validate();

	atom::Kernel kernel(atom::Kernel::eAll);
	kernel.atoms.resize(pointer);
	std::memcpy(kernel.atoms.data(), pool.data(), sizeof(atom::General) * pointer);
	kernel.synthesized = synthesized;
	kernel.used = used;

	// No longer "owns" the IR
	clear();

	return kernel;
}

// Printing the IR state
void Emitter::dump()
{
	fmt::println("------------------------------");
	fmt::println("GLOBALS ({}/{})", pointer, pool.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++) {
		if (synthesized.contains(i))
			fmt::print("S");
		else
			fmt::print(" ");

		if (used.contains(i))
			fmt::print("U");
		else
			fmt::print(" ");

		fmt::print(" [{:4d}]: ", i);
			atom::dump_ir_operation(pool[i]);
		fmt::print("\n");
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
