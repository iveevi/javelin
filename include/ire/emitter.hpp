#pragma once

#include <stack>
#include <unordered_set>

#include "atom.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

struct Emitter {
	// By default the program begins at index=0
	atom::General *pool;
	atom::General *dual;

	size_t size;
	size_t pointer;

	std::stack <int> control_flow_ends;
	std::unordered_set <int> used;
	std::unordered_set <int> synthesized;

	Emitter();

	// Resizing and compaction
	void clear();
	void compact();
	void resize(size_t);

	// Dead code elimination, always conservative
	void mark_used(int, bool);
	void mark_synthesized_underlying(int);

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

	// Generating GLSL source code
	std::string generate_glsl();

	// Printing the IR state
	void dump();

	static thread_local Emitter active;
};

namespace detail {

std::tuple <size_t, wrapped::reindex>
ir_compact_deduplicate(const atom::General *const,
		       atom::General *const,
		       std::unordered_set <int> &,
		       size_t);

} // namespace detail

} // namespace jvl::ire
