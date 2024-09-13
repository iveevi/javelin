#pragma once

#include <libgccjit.h>

#include "buffer.hpp"

namespace jvl::thunder::detail {

struct gcc_jit_function_generator_t : Buffer {
	gcc_jit_context *const context;
	gcc_jit_function *function;
	gcc_jit_block *block;

	std::vector <gcc_jit_param *> parameters;

	wrapped::hash_table <index_t, gcc_jit_object *> values;
	wrapped::hash_table <QualifiedType, gcc_jit_type *> mapped_types;

	gcc_jit_function_generator_t(gcc_jit_context *const , const Buffer &);

	// Generating types
	gcc_jit_type *jitify_type(QualifiedType);
	
	// Per-atom generator
	void generate(index_t);

	template <atom_instruction T>
	gcc_jit_object *generate(const T &atom, index_t i) {
		MODULE(gcc-jit-generate);

		JVL_ABORT("failed to JIT (gcc-jit) compile atom: {} (@{})", atom, i);
	}
	
	gcc_jit_object *load_field(index_t, index_t, bool);
	
	// Expand generation list
	auto work_list();

	// Configure the function
	void begin_function();

	// Wholistic generation
	void generate();
};

} // namespace jvl::thunder::detail