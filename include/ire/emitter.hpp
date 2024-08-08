#pragma once

#include <stack>
#include <unordered_set>

#include "op.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

struct Emitter {
	// By default the program begins at index=0
	op::General *pool;
	op::General *dual;

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
	int emit(const op::General &);

	int emit_main(const op::General &);
	int emit_main(const op::Cond &);
	int emit_main(const op::Elif &);
	int emit_main(const op::While &);
	int emit_main(const op::End &);

	void control_flow_callback(int, int);

	// Generating GLSL source code
	std::string generate_glsl();

	// Printing the IR state
	void dump();

	static thread_local Emitter active;
};

namespace detail {

std::tuple <size_t, wrapped::reindex>
ir_compact_deduplicate(const op::General *const,
		       op::General *const,
		       std::unordered_set <int> &,
		       size_t);

} // namespace detail

} // namespace jvl::ire
