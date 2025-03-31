#pragma once

#include <stack>

#include "../common/logging.hpp"
#include "buffer.hpp"

namespace jvl::thunder {

// With linear IR each scope is just a range...
struct Scope {
	Index begin;
	Index end;

	bool contains(Index x) const {
		return (x >= begin) && (x <= end);
	}

	operator bool() const {
		return (begin > 0) && (end > begin);
	}

	static const Scope &invalid() {
		static constexpr Scope null(-1, -1);
		return null;
	}
};

std::string format_as(const Scope &scope)
{
	return fmt::format("[{}, {}]", scope.begin, scope.end);
}

// ...and a scope hierarchy is essentially a refinement
struct ScopeHierarchy : Scope {
	std::vector <ScopeHierarchy> nested;

	ScopeHierarchy() = default;
	ScopeHierarchy(Index a, Index b) : Scope(a, b) {}

	Scope lifetime(Index x) const {
		// Completely out of range...
		if (!contains(x))
			return Scope::invalid();

		// Contained in a smaller nested scope
		for (auto &scope : nested) {
			if (auto lf = scope.lifetime(x))
				return lf;
		}

		// Contained just in this scope
		return Scope(x, end);
	}

	// Returns reference to that addresses give unique blocks
	const Scope &block(Index x) const {
		// Similar logic to above...
		if (!contains(x))
			return Scope::invalid();

		for (auto &scope : nested) {
			if (auto &lf = scope.block(x))
				return lf;
		}

		// ... except we don't need to cut it off
		return *this;
	}

	void display(int32_t ind = 0) const {
		static constexpr int32_t spacing = 4;

		std::string tab(spacing * ind, ' ');

		fmt::println("{}{}", tab, static_cast <const Scope &> (*this));
		for (const auto &scope : nested)
			scope.display(ind + 1);
	}
};

// struct Block : Scope {
// };

// TODO: CFG construction...
// TODO: flesh this out at some point with connections and phis...
ScopeHierarchy cfg_blocks(const Buffer &buffer)
{
	MODULE(cfg_blocks);

	std::stack <ScopeHierarchy> stack;

	stack.emplace(0, -1);
	
	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];
		auto &scope = stack.top();
		scope.end = i;

		if (auto branch = atom.get <Branch> ()) {
			switch (branch->kind) {

			case loop_while:
			{
				stack.emplace(i, -1);
			} break;

			case control_flow_end:
			{
				auto done = stack.top();
				stack.pop();

				JVL_ASSERT(!stack.empty(), "scope stack is unexpectedly empty");

				auto &higher = stack.top();
				higher.nested.emplace_back(done);
			} break;

			default:
				JVL_ABORT("unhandled branch kind:\n{}", atom);
			}
		}
	}

	JVL_ASSERT(stack.size() == 1,
		"unexpected number of scopes remaining "
		"({} instead of just one)", stack.size());

	return stack.top();
}

} // namespace jvl::thunder