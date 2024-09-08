#include <cassert>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "ire/emitter.hpp"
#include "thunder/opt.hpp"
#include "wrapped_types.hpp"
#include "logging.hpp"

namespace jvl::ire {

MODULE(emitter);

Emitter::Emitter() : Scratch() {}

// TODO: every level should have be a scratch
void Emitter::clear()
{
	Scratch::clear();
}

// Scope management
void Emitter::push(Scratch &scratch)
{
	scopes.push(std::ref(scratch));
	JVL_INFO("pushed new scratch buffer to global emitter ({} scopes)", scopes.size());
}

void Emitter::pop()
{
	scopes.pop();
	JVL_INFO("popped scratch buffer from global emitter ({} scopes)", scopes.size());
}

// Emitting instructions during function invocation
Emitter::index_t Emitter::emit(const thunder::Atom &atom)
{
	if (scopes.size())
		return scopes.top().get().emit(atom);

	return Scratch::emit(atom);
}

Emitter::index_t Emitter::emit(const thunder::Branch &branch)
{
	int p = emit((thunder::Atom) branch);
	control_flow_ends.push(p);
	return p;
}

Emitter::index_t Emitter::emit(const thunder::While &loop)
{
	int p = emit((thunder::Atom) loop);
	control_flow_ends.push(p);
	return p;
}

Emitter::index_t Emitter::emit(const thunder::End &end)
{
	int p = emit((thunder::Atom) end);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	return p;
}

Emitter::index_t Emitter::emit_list_chain(const std::vector <index_t> &atoms)
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
	// TODO: export the kernel, then optimize and dce, and then validate...
	// validate();

	// TODO: run through optimizations

	thunder::Kernel kernel(thunder::Kernel::eAll);
	kernel.atoms = pool;
	kernel.atoms.resize(pointer);

	// No longer "owns" the IR
	clear();

	return kernel;
}

// Printing the IR state
void Emitter::dump()
{
	if (scopes.empty()) {
		fmt::println("------------------------------");
		fmt::println("GLOBALS ({}/{})", pointer, pool.size());
		fmt::println("------------------------------");
		for (size_t i = 0; i < pointer; i++)
			fmt::println("  [{:4d}]: {}", i, pool[i].to_string());
	} else {
		auto &scratch = scopes.top().get();
		fmt::println("------------------------------");
		fmt::println("GLOBALS-SCRATCH ({}/{})", scratch.pointer, scratch.pool.size());
		fmt::println("------------------------------");
		for (size_t i = 0; i < scratch.pointer; i++)
			fmt::println("  [{:4d}]: {}", i, scratch.pool[i].to_string());
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
