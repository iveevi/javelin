#include <cassert>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "core/logging.hpp"
#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

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
Emitter::Index Emitter::emit(const thunder::Atom &atom)
{
	JVL_ASSERT(scopes.size(), "in emit: no active scope");
	return scopes.top().get().emit(atom, classify.top());
}

Emitter::Index Emitter::emit(const thunder::Branch &branch, const precondition_t &pre)
{
	auto &buffer = scopes.top().get();

	Index i = -1;
	if (branch.kind != thunder::control_flow_end)
		i = emit((thunder::Atom) branch);

	switch (branch.kind) {

	case thunder::conditional_if:
		control_flow_ends.push(cf_await(i));
		return i;

	case thunder::conditional_else:
	case thunder::conditional_else_if:
	{
		auto await = control_flow_ends.top();
		control_flow_ends.pop();

		auto &prior = buffer[await.ref];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;

		if (branch.kind != thunder::conditional_else)
			control_flow_ends.push(cf_await(i));

		// else branch should not be waiting...
	} return i;

	case thunder::loop_while:
		control_flow_ends.push(cf_await(i, pre));
		return i;

	case thunder::control_flow_end:
	{
		auto await = control_flow_ends.top();
		control_flow_ends.pop();

		if (await.pre)
			await.pre.value()();

		i = emit((thunder::Atom) branch);

		auto &prior = buffer[await.ref];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;
	} return i;

	// These two are simply keywords
	case thunder::control_flow_skip:
	case thunder::control_flow_stop:
		return i;

	default:
		break;
	}

	JVL_ABORT("unhandled case of control_flow_callback: {}", branch);
}

Emitter::Index Emitter::emit_list_chain(const std::vector <Index> &atoms)
{
	thunder::List list;

	Index next = -1;
	for (auto it = std::rbegin(atoms); it != std::rend(atoms); it++) {
		list.item = *it;
		list.next = next;
		next = emit(list);
	}

	return next;
}

// Printing the IR state
void Emitter::dump()
{
	JVL_ASSERT(scopes.size(), "no active scope in {}", __FUNCTION__);

	auto &buffer = scopes.top().get();
	fmt::println("------------------------------");
	fmt::println("BUFFER IN PROGRESS ({}/{})", buffer.pointer, buffer.atoms.size());
	fmt::println("------------------------------");
	buffer.dump();
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
