#pragma once

#include <functional>
#include <initializer_list>
#include <stack>

#include "../thunder/buffer.hpp"
#include "../thunder/atom.hpp"
#include "../thunder/kernel.hpp"
#include "../thunder/enumerations.hpp"

namespace jvl::ire {

// More advanced pool which manages control flow as well as scopes of pools
struct Emitter {
	using index_t = thunder::index_t;

	std::stack <bool> classify;
	std::stack <index_t> control_flow_ends;
	std::stack <std::reference_wrapper <thunder::Buffer>> scopes;

	Emitter() = default;

	// Managing the scope
	void push(thunder::Buffer &, bool = true);
	void pop();

	// Emitting instructions during function invocation
	index_t emit(const thunder::Atom &);
	index_t emit(const thunder::Branch &);

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

	index_t emit_operation(index_t args, index_t type, thunder::OperationCode code) {
		return emit(thunder::Operation(args, type, code));
	}

	index_t emit_intrinsic(index_t args, index_t type, thunder::IntrinsicOperation opn) {
		return emit(thunder::Intrinsic(args, type, opn));
	}

	index_t emit_list(index_t item, index_t next) {
		return emit(thunder::List(item, next));
	}

	index_t emit_construct(index_t type, index_t args, bool transient) {
		return emit(thunder::Construct(type, args, transient));
	}

	index_t emit_call(index_t cid, index_t args, index_t type) {
		return emit(thunder::Call(cid, args, type));
	}

	index_t emit_store(index_t dst, index_t src, bool bss) {
		return emit(thunder::Store(dst, src, bss));
	}

	index_t emit_load(index_t src, index_t idx) {
		return emit(thunder::Load(src, idx));
	}

	index_t emit_branch(index_t cond, index_t failto, thunder::BranchKind kind) {
		return emit(thunder::Branch(cond, failto, kind));
	}

	index_t emit_return(index_t value, index_t type) {
		return emit(thunder::Returns(value, type));
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

	// Callbacks for control flow types
	void control_flow_callback(index_t);

	// Printing the IR state
	void dump();

	static thread_local Emitter active;
};

template <typename F, typename ... Args>
thunder::Kernel kernel_from_args(const F &ftn, const Args &... args)
{
	auto &em = Emitter::active;
	thunder::Buffer scratch;
	em.push(scratch);
		ftn(args...);
	em.pop();
	return scratch.export_to_kernel();
}

} // namespace jvl::ire
