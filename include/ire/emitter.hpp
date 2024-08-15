#pragma once

#include <functional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "../atom/atom.hpp"
#include "../atom/kernel.hpp"
#include "../wrapped_types.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// TODO: move to another header
struct Callable {
	// Global list of callables
	static auto &tracked() {
		static std::unordered_map <size_t, Callable *> map;
		return map;
	}

	// Ordinary information, same as Emitter
	// but lacks the used and synthesized members
	std::vector <atom::General> pool;
	size_t pointer;
	size_t cid;

	// For callables we can track back used and synthesized
	// insructions from working backwards at the returns

	Callable();
	Callable(const Callable &);
	Callable &operator=(const Callable &);

	// TODO: destructor, which offloads it from the global list

	atom::Kernel export_to_kernel();

	int emit(const atom::General &);

	void dump();
};

struct Emitter {
	// By default the program begins at index 0
	std::vector <atom::General> pool;
	size_t pointer;

	std::stack <atom::index_t> control_flow_ends;
	std::unordered_set <atom::index_t> used;
	std::unordered_set <atom::index_t> synthesized;

	std::stack <std::reference_wrapper <Callable>> scopes;

	Emitter();

	// Resizing and compaction
	void clear();
	void compact();

	// Dead code elimination, always conservative
	void mark_used(int, bool);

	// Emitting instructions during function invocation
	int emit(const atom::General &);

	int emit_main(const atom::General &);
	int emit_main(const atom::Cond &);
	int emit_main(const atom::Elif &);
	int emit_main(const atom::While &);
	int emit_main(const atom::End &);

	void control_flow_callback(int, int);

	// Validating IR
	// TODO: pass stage
	void validate() const;

	// Transfering to a Kernel object
	atom::Kernel export_to_kernel();

	// Printing the IR state
	void dump();

	static thread_local Emitter active;
};

template <typename F, typename ... Args>
atom::Kernel kernel_from_args(const F &ftn, const Args &... args)
{
	Emitter::active.clear();
	ftn(args...);
	return Emitter::active.export_to_kernel();
}

namespace detail {

std::tuple <size_t, wrapped::reindex <atom::index_t>>
ir_compact_deduplicate(const atom::General *const,
		       atom::General *const,
		       std::unordered_set <atom::index_t> &,
		       size_t);

} // namespace detail

} // namespace jvl::ire
