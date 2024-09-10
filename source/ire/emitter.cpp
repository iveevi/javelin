#include <cassert>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/opt.hpp"
#include "wrapped_types.hpp"
#include "logging.hpp"

namespace jvl::ire {

MODULE(emitter);

// Scope management
void Emitter::push(thunder::Buffer &scratch)
{
	scopes.push(std::ref(scratch));
	// JVL_INFO("pushed new scratch buffer to global emitter ({} scopes)", scopes.size());
}

void Emitter::pop()
{
	scopes.pop();
	// JVL_INFO("popped scratch buffer from global emitter ({} scopes)", scopes.size());
}

// Emitting instructions during function invocation
Emitter::index_t Emitter::emit(const thunder::Atom &atom)
{
	JVL_ASSERT(scopes.size(), "in emit: no active scope");
	return scopes.top().get().emit(atom);
}

Emitter::index_t Emitter::emit(const thunder::Branch &branch)
{
	JVL_INFO("emitting branch instruction");

	// TODO: check else if and loops
	index_t i = emit((thunder::Atom) branch);
	control_flow_ends.push(i);
	return i;
}

Emitter::index_t Emitter::emit(const thunder::While &loop)
{
	JVL_ABORT("emitting for loops not implemented");
	return -1;
}

Emitter::index_t Emitter::emit(const thunder::End &end)
{
	JVL_ASSERT(control_flow_ends.size(), "end instruction called without an existing control flow");

	index_t i = emit((thunder::Atom) end);
	index_t ref = control_flow_ends.top();
	JVL_INFO("emitting end instruction @{} to close ref @{}", i, ref);

	control_flow_callback(ref, i);
	control_flow_ends.pop();
	return i;
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
	auto &atom = scopes.top().get().pool[ref];
	// TODO: branch classification
	JVL_ASSERT(atom.is <thunder::Branch> (),
		"callback location (scoped) is not a branch: {} (@{})",
		atom, ref);
	atom.as <thunder::Branch> ().failto = p;
}

// Printing the IR state
void Emitter::dump()
{
	JVL_ASSERT(scopes.size(), "in dump: no active scope");

	auto &buffer = scopes.top().get();
	fmt::println("------------------------------");
	fmt::println("BUFFER IN PROGRESS ({}/{})", buffer.pointer, buffer.pool.size());
	fmt::println("------------------------------");
	buffer.dump();
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire