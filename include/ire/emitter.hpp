#pragma once

#include <functional>
#include <initializer_list>
#include <stack>

#include "../thunder/buffer.hpp"
#include "../thunder/atom.hpp"
#include "../thunder/enumerations.hpp"

namespace jvl::ire {

// More advanced pool which manages control flow as well as scopes of pools
struct Emitter {
	using Index = thunder::Index;
	using precondition_t = std::optional <std::function <void ()>>;
	using buffer_ref_t = std::reference_wrapper <thunder::Buffer>;

	struct cf_await {
		Index ref;
		precondition_t pre;
	};

	std::stack <bool> classify;
	std::stack <cf_await> control_flow_ends;
	std::stack <buffer_ref_t> scopes;

	Emitter() = default;

	// Managing the scope
	void push(thunder::Buffer &, bool = true);
	void pop();

	// Emitting type hints
	void emit_hint(Index idx, uint64_t id, const std::string &name, const std::vector <std::string> &fields) {
		auto &buf = scopes.top().get();
		if (!buf.decorations.contains(id)) {
			auto th = thunder::Buffer::type_hint(name, fields);
			buf.decorations[id] = th;
		}

		buf.used_decorations[idx] = id;
	}

	// Emitting instructions during function invocation
	Index emit(const thunder::Atom &);
	Index emit(const thunder::Branch &, const precondition_t & = std::nullopt);

	// Instruction specific emitters
	Index emit_qualifier(Index underlying, Index numerical, thunder::QualifierKind kind) {
		return emit(thunder::Qualifier(underlying, numerical, kind));
	}

	Index emit_type_information(Index down, Index next, thunder::PrimitiveType item) {
		return emit(thunder::TypeInformation(down, next, item));
	}

	template <typename T>
	Index emit_primitive(T value) {
		thunder::Primitive primitive;
		if constexpr (std::same_as <T, bool>) {
			primitive.bdata = value;
			primitive.type = thunder::boolean;
		} else if constexpr (std::same_as <T, int32_t>) {
			primitive.idata = value;
			primitive.type = thunder::i32;
		} else if constexpr (std::same_as <T, uint32_t>) {
			primitive.udata = value;
			primitive.type = thunder::u32;
		} else if constexpr (std::same_as <T, uint64_t>) {
			primitive.udata = value;
			primitive.type = thunder::u64;
		} else if constexpr (std::same_as <T, float>) {
			primitive.fdata = value;
			primitive.type = thunder::f32;
		}

		return emit(primitive);
	}

	Index emit_swizzle(Index src, thunder::SwizzleCode code) {
		return emit(thunder::Swizzle(src, code));
	}

	Index emit_operation(Index a, Index b, thunder::OperationCode code) {
		return emit(thunder::Operation(a, b, code));
	}

	Index emit_intrinsic(Index args, thunder::IntrinsicOperation opn) {
		return emit(thunder::Intrinsic(args, opn));
	}

	Index emit_list(Index item, Index next = -1) {
		return emit(thunder::List(item, next));
	}

	Index emit_construct(Index type, Index args, thunder::ConstructorMode mode) {
		return emit(thunder::Construct(type, args, mode));
	}

	Index emit_call(Index cid, Index args, Index type) {
		return emit(thunder::Call(cid, args, type));
	}

	Index emit_store(Index dst, Index src) {
		return emit(thunder::Store(dst, src));
	}

	Index emit_load(Index src, Index idx) {
		return emit(thunder::Load(src, idx));
	}
	
	Index emit_array_access(Index src, Index loc) {
		return emit(thunder::ArrayAccess(src, loc));
	}

	Index emit_branch(Index cond, Index failto, thunder::BranchKind kind, const precondition_t &pre = std::nullopt) {
		return emit(thunder::Branch(cond, failto, kind), pre);
	}

	Index emit_return(Index value) {
		return emit(thunder::Returns(value));
	}

	// Easier ways to construct list chains
	Index emit_list_chain(const std::vector <Index> &);

	template <std::integral ... Args>
	Index emit_list_chain(const Args &... args) {
		return emit_list_chain(std::initializer_list <Index> { args... });
	}
	
	// Printing the active buffer
	void dump();

	static thread_local Emitter active;
};

} // namespace jvl::ire
