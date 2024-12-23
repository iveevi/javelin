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
	using index_t = thunder::index_t;
	using precondition_t = std::optional <std::function <void ()>>;
	using buffer_ref_t = std::reference_wrapper <thunder::Buffer>;

	struct cf_await {
		index_t ref;
		precondition_t pre;
	};

	std::stack <bool> classify;
	std::stack <cf_await> control_flow_ends;
	std::stack <buffer_ref_t> scopes;

	Emitter() = default;

	// Managing the scope
	void push(thunder::Buffer &, bool = true);
	void pop();

	// Emitting instructions during function invocation
	index_t emit(const thunder::Atom &);
	index_t emit(const thunder::Branch &, const precondition_t & = std::nullopt);

	// Instruction specific emitters
	index_t emit_qualifier(index_t underlying, index_t numerical, thunder::QualifierKind kind) {
		return emit(thunder::Qualifier(underlying, numerical, kind));
	}

	index_t emit_type_information(index_t down, index_t next, thunder::PrimitiveType item) {
		return emit(thunder::TypeInformation(down, next, item));
	}

	template <typename T>
	index_t emit_primitive(T value) {
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
		} else if constexpr (std::same_as <T, float>) {
			primitive.fdata = value;
			primitive.type = thunder::f32;
		}

		return emit(primitive);
	}

	index_t emit_swizzle(index_t src, thunder::SwizzleCode code) {
		return emit(thunder::Swizzle(src, code));
	}

	index_t emit_operation(index_t a, index_t b, thunder::OperationCode code) {
		return emit(thunder::Operation(a, b, code));
	}

	index_t emit_intrinsic(index_t args, thunder::IntrinsicOperation opn) {
		return emit(thunder::Intrinsic(args, opn));
	}

	index_t emit_list(index_t item, index_t next = -1) {
		return emit(thunder::List(item, next));
	}

	index_t emit_construct(index_t type, index_t args, thunder::ConstructorMode mode) {
		return emit(thunder::Construct(type, args, mode));
	}

	index_t emit_call(index_t cid, index_t args, index_t type) {
		return emit(thunder::Call(cid, args, type));
	}

	index_t emit_store(index_t dst, index_t src) {
		return emit(thunder::Store(dst, src));
	}

	index_t emit_load(index_t src, index_t idx) {
		return emit(thunder::Load(src, idx));
	}
	
	index_t emit_array_access(index_t src, index_t loc) {
		return emit(thunder::ArrayAccess(src, loc));
	}

	index_t emit_branch(index_t cond, index_t failto, thunder::BranchKind kind, const precondition_t &pre = std::nullopt) {
		return emit(thunder::Branch(cond, failto, kind), pre);
	}

	index_t emit_return(index_t value) {
		return emit(thunder::Returns(value));
	}

	// Easier ways to construct list chains
	index_t emit_list_chain(const std::vector <index_t> &);

	template <std::integral ... Args>
	index_t emit_list_chain(const Args &... args) {
		return emit_list_chain(std::initializer_list <index_t> { args... });
	}

	// Easier ways to emit multiple instructions in a sequence
	std::vector <index_t> emit_sequence(const std::initializer_list <thunder::Atom> &);

	template <size_t N, size_t I, thunder::atom_instruction T, thunder::atom_instruction ... Args>
	void __emit_sequence(std::array <index_t, N> &result, const T &t, const Args &... args) {
		result[I] = emit(t);
		if constexpr (I + 1 < N)
			__emit_sequence <N, I + 1> (result, args...);
	}

	template <thunder::atom_instruction T, thunder::atom_instruction ... Args>
	auto emit_sequence(const T &t, const Args &... args) {
		constexpr size_t N = sizeof...(Args) + 1;
		std::array <index_t, N> result;
		__emit_sequence <N, 0> (result, t, args...);
		return result;
	}

	// Printing the active buffer
	void dump();

	static thread_local Emitter active;
};

} // namespace jvl::ire
