#pragma once

#include <functional>
#include <initializer_list>
#include <stack>

#include "../thunder/buffer.hpp"
#include "../thunder/atom.hpp"
#include "../thunder/enumerations.hpp"

namespace jvl::ire {

// More advanced pool which manages control flow as well as scopes of pools
class Emitter {
	thunder::Buffer &buffer();
public:
	using Index = thunder::Index;
	using precondition = std::optional <std::function <void ()>>;
	using reference = std::reference_wrapper <thunder::Buffer>;

	// TODO: move inside, capitalize type...
	struct cf_await {
		Index ref;
		precondition pre;
	};

	// TODO: move inside as well
	std::stack <bool> classify;
	std::stack <cf_await> control_flow_ends;
	std::stack <reference> scopes;

	Emitter() = default;

	// Managing the scope
	void push(thunder::Buffer &, bool = true);
	void pop();

	// Emitting instructions during function invocation
	Index emit(const thunder::Atom &);

	// Qualifiers
	Index emit_qualifier(Index, Index, thunder::QualifierKind);

	// Type information
	Index emit_type_information(Index, Index, thunder::PrimitiveType);
	
	// Primitives
	Index emit_primitive(bool);
	Index emit_primitive(int32_t);
	Index emit_primitive(uint32_t);
	Index emit_primitive(uint64_t);
	Index emit_primitive(float);

	// Swizzles
	Index emit_swizzle(Index, thunder::SwizzleCode);

	// Operations
	Index emit_operation(Index, Index, thunder::OperationCode);

	// Intrinsics
	Index emit_intrinsic(Index, thunder::IntrinsicOperation);

	// List chains
	Index emit_list(Index, Index = -1);

	// Constructions: type, args, mode
	Index emit_construct(Index, Index, thunder::ConstructorMode);

	// Invocations
	Index emit_call(Index, Index, Index);

	// Stores
	Index emit_store(Index, Index);

	// Loads
	Index emit_load(Index, Index);
	
	// Indexing
	Index emit_array_access(Index, Index);

	// Branches
	Index emit_branch(Index, Index, thunder::BranchKind, const precondition & = std::nullopt);

	Index emit(const thunder::Branch &, const precondition & = std::nullopt);

	// Returns
	Index emit_return(Index);

	// Easier ways to construct list chains
	Index emit_list_chain(const std::vector <Index> &);

	template <std::integral ... Args>
	Index emit_list_chain(const Args &... args) {
		return emit_list_chain(std::initializer_list <Index> { args... });
	}

	// Emitting type hints
	void add_type_hint(Index, uint64_t, const std::string &, const std::vector <std::string> &);
	void add_phantom_hint(Index);
	
	// Printing the active buffer
	void display_pretty();

	// Per-thread singleton
	static thread_local Emitter active;
};

} // namespace jvl::ire
