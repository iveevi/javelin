#include <cassert>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "wrapped_types.hpp"
#include "logging.hpp"

namespace jvl::ire {

MODULE(emitter);

// Scope management
void Emitter::push(thunder::Buffer &scratch, bool enable_classify)
{
	scopes.push(std::ref(scratch));
	classify.push(enable_classify);
	// JVL_INFO("pushed new scratch buffer to global emitter ({} scopes)", scopes.size());
}

void Emitter::pop()
{
	scopes.pop();
	classify.pop();
	// JVL_INFO("popped scratch buffer from global emitter ({} scopes)", scopes.size());
}

// Emitting instructions during function invocation
Emitter::index_t Emitter::emit(const thunder::Atom &atom)
{
	JVL_ASSERT(scopes.size(), "in emit: no active scope");
	return scopes.top().get().emit(atom, classify.top());
}

Emitter::index_t Emitter::emit(const thunder::Branch &branch)
{
	// TODO: check else if and loops
	// TODO: check for end
	index_t i = emit((thunder::Atom) branch);
	control_flow_callback(i);
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

void Emitter::control_flow_callback(index_t i)
{
	auto &buffer = scopes.top().get();

	auto &atom = buffer[i];

	JVL_ASSERT(atom.is <thunder::Branch> (),
		"origin of control flow callback must be a branch: {}",
		atom);

	auto &branch = atom.as <thunder::Branch> ();

	switch (branch.kind) {

	case thunder::conditional_if:
	case thunder::loop_while:
		return control_flow_ends.push(i);

	case thunder::conditional_else:
	case thunder::conditional_else_if:
	{
		index_t failto = control_flow_ends.top();
		control_flow_ends.pop();

		auto &prior = buffer[failto];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;

		return control_flow_ends.push(i);
	}

	case thunder::control_flow_end:
	{
		index_t failto = control_flow_ends.top();
		control_flow_ends.pop();

		auto &prior = buffer[failto];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;

		return;
	}

	default:
		break;
	}
		
	JVL_ABORT("unhandled case of control_flow_callback: {}", atom);
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