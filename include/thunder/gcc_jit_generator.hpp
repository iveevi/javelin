#pragma once

#include <libgccjit.h>

#include "buffer.hpp"

namespace jvl::thunder::detail {

// Type information with size and alignment info
struct gcc_type_info {
	gcc_jit_type *real = nullptr;
	uint32_t size = 0;
	uint32_t align = 0;
};

struct gcc_jit_function_generator_t : Buffer {
	gcc_jit_context *const context;
	gcc_jit_function *function;
	gcc_jit_block *block;

	std::vector <gcc_jit_param *> parameters;

	wrapped::hash_table <index_t, gcc_jit_object *> values;
	wrapped::hash_table <QualifiedType, gcc_type_info> mapped_types;

	gcc_jit_function_generator_t(gcc_jit_context *const , const Buffer &);

	// Generating types
	gcc_type_info jitify_type(QualifiedType);

	// Expanding list chains
	struct expanded_list_chain {
		std::vector <PrimitiveType> types;
		std::vector <gcc_jit_rvalue *> rvalues;
	};

	expanded_list_chain expand_list_chain(index_t) const;
	
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