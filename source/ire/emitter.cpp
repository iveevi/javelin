#include <cassert>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "ire/emitter.hpp"
#include "thunder/opt.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

Emitter::Emitter() : Scratch() {}

// TODO: every level should have be a scratch
void Emitter::clear()
{
	// Reset usages
	used.clear();
	synthesized.clear();

	Scratch::clear();

	for (auto &cb : scoping_callbacks)
		cb();
}

// Scope management
void Emitter::push(Scratch &scratch)
{
	for (auto &cb : scoping_callbacks)
		cb();

	scopes.push(std::ref(scratch));
}

void Emitter::pop()
{
	for (auto &cb : scoping_callbacks)
		cb();

	scopes.pop();
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
Emitter::index_t Emitter::emit(const thunder::Atom &atom)
{
	if (scopes.size())
		return scopes.top().get().emit(atom);

	return Scratch::emit(atom);
}

Emitter::index_t Emitter::emit_main(const thunder::Atom &atom)
{
	int p = emit(atom);
	synthesized.insert(p);
	mark_used(p, true);
	return p;
}

Emitter::index_t Emitter::emit_main(const thunder::Branch &branch)
{
	int p = emit_main((thunder::Atom) branch);
	control_flow_ends.push(p);
	return p;
}

// TODO: fix the state of branching
// int Emitter::emit_main(const thunder::Elif &elif)
// {
// 	int p = emit_main((thunder::Atom) elif);
// 	assert(control_flow_ends.size());
// 	auto ref = control_flow_ends.top();
// 	control_flow_ends.pop();
// 	control_flow_callback(ref, p);
// 	control_flow_ends.push(p);
// 	return p;
// }

Emitter::index_t Emitter::emit_main(const thunder::While &loop)
{
	int p = emit_main((thunder::Atom) loop);
	control_flow_ends.push(p);
	return p;
}

Emitter::index_t Emitter::emit_main(const thunder::End &end)
{
	int p = emit_main((thunder::Atom) end);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	return p;
}

Emitter::index_t Emitter::emit_list_chain(const std::initializer_list <index_t> &atoms)
{
	thunder::List list;

	index_t next = -1;
	for (auto it = std::rbegin(atoms); it != std::rend(atoms); it++) {
		list.item = *it;
		list.next = next;
		next = emit(list);
	}

	return next;
}

std::vector <Emitter::index_t> Emitter::emit_sequence(const std::initializer_list <thunder::Atom> &atoms)
{
	std::vector <index_t> result;
	for (auto &atom : atoms)
		result.push_back(emit(atom));

	return result;
}

void Emitter::control_flow_callback(int ref, int p)
{
	thunder::Atom op;
	if (scopes.size())
		op = scopes.top().get().pool[ref];
	else
		op = pool[ref];

	if (op.is <thunder::Branch> ()) {
		op.as <thunder::Branch> ().failto = p;
	} else if (op.is <thunder::While> ()) {
		op.as <thunder::While> ().failto = p;
	} else {
		fmt::print("op not conditional, is actually: {}", op.to_string());
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

// Kernel transfer
thunder::Kernel Emitter::export_to_kernel()
{
	validate();

	// TODO: run through optimizations

	thunder::Kernel kernel(thunder::Kernel::eAll);
	kernel.atoms = pool;
	kernel.atoms.resize(pointer);
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

		fmt::println(" [{:4d}]: {}", i, pool[i].to_string());
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
