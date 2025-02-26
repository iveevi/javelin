// Native JIT libraries
#include <libgccjit.h>

#include "thunder/gcc_jit_generator.hpp"
#include "thunder/linkage_unit.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

//////////////////////////////////////////
// Generation: JIT compilation with GCC //
//////////////////////////////////////////

void *LinkageUnit::generate_jit_gcc() const
{
	JVL_INFO("compiling linkage atoms with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	JVL_ASSERT(context, "failed to acquire context");

	// TODO: pass options
	gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);
	gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DEBUGINFO, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_TREE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	for (auto &function : functions) {
		// detail::unnamed_body_t body(block);
		detail::gcc_jit_function_generator_t generator(context, function);
		generator.generate();
	}

	gcc_jit_context_dump_to_file(context, "gcc_jit_result.c", true);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	JVL_ASSERT(result, "failed to compile function");

	void *ftn = gcc_jit_result_get_code(result, "function");
	JVL_ASSERT(result, "failed to load function result");

	JVL_INFO("successfully JIT-ed linkage unit");

	return ftn;
}

} // namespace jvl::thunder
