#pragma once

#include <functional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "../thunder/atom.hpp"
#include "../thunder/kernel.hpp"
#include "../wrapped_types.hpp"
#include "tagged.hpp"

namespace jvl::ire {

struct Callable;

struct Emitter {
	// By default the program begins at index 0
	std::vector <thunder::Atom> pool;
	size_t pointer;

	std::stack <thunder::index_t> control_flow_ends;
	std::unordered_set <thunder::index_t> used;
	std::unordered_set <thunder::index_t> synthesized;

	std::stack <std::reference_wrapper <Callable>> scopes;

	Emitter();

	// Resizing and compaction
	void clear();
	void compact();

	// Dead code elimination, always conservative
	void mark_used(int, bool);

	// Emitting instructions during function invocation
	int emit(const thunder::Atom &);

	int emit_main(const thunder::Atom &);
	int emit_main(const thunder::Cond &);
	int emit_main(const thunder::Elif &);
	int emit_main(const thunder::While &);
	int emit_main(const thunder::End &);

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
