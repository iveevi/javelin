#pragma once

#include <functional>
#include <initializer_list>
#include <stack>
#include <unordered_set>

#include "../thunder/atom.hpp"
#include "../thunder/kernel.hpp"
#include "../wrapped_types.hpp"

namespace jvl::ire {

// Arbitrary pools of atoms
struct Scratch {
	using index_t = thunder::index_t;

	std::vector <thunder::Atom> pool;
	size_t pointer;

	// TODO: buffering with a static sized array

	Scratch();

	void reserve(size_t);
	
	index_t emit(const thunder::Atom &);

	void clear();

	void dump();
};

// More advanced pool which manages control flow as well as scopes of pools
struct Emitter : Scratch {
	std::stack <thunder::index_t> control_flow_ends;
	std::unordered_set <thunder::index_t> used;
	std::unordered_set <thunder::index_t> synthesized;

	std::stack <std::reference_wrapper <Scratch>> scopes;

	Emitter();

	// Resizing and compaction
	void clear();

	// TODO: compaction is really optimization
	void compact();

	// Managing the scope
	void push(Scratch &);
	void pop();

	// Dead code elimination, always conservative
	void mark_used(int, bool);

	// Emitting instructions during function invocation
	index_t emit(const thunder::Atom &);

	index_t emit_main(const thunder::Atom &);
	index_t emit_main(const thunder::Branch &);
	index_t emit_main(const thunder::While &);
	index_t emit_main(const thunder::End &);

	// Easier ways to construct list chains
	index_t emit_list_chain(const std::initializer_list <index_t> &);

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
	void control_flow_callback(int, int);

	// Validating IR
	// TODO: pass stage
	void validate() const;

	// Transfering to a Kernel object
	thunder::Kernel export_to_kernel();

	// Printing the IR state
	void dump();

	static thread_local Emitter active;
};

template <typename F, typename ... Args>
thunder::Kernel kernel_from_args(const F &ftn, const Args &... args)
{
	Emitter::active.clear();
	ftn(args...);
	return Emitter::active.export_to_kernel();
}

namespace detail {

std::tuple <size_t, wrapped::reindex <thunder::index_t>>
ir_compact_deduplicate(const thunder::Atom *const,
		       thunder::Atom *const,
		       std::unordered_set <thunder::index_t> &,
		       size_t);

void mark_used(const std::vector <thunder::Atom> &,
	       std::unordered_set <thunder::index_t> &,
	       std::unordered_set <thunder::index_t> &,
	       int, bool);

} // namespace detail

} // namespace jvl::ire
