#include <gtest/gtest.h>

#include "ire/core.hpp"
#include "thunder/scratch.hpp"
#include "thunder/scratch.hpp"

using namespace jvl;
using namespace jvl::ire;

// Checking that certain operations appear in IR
bool ir_cmp_op(const thunder::Atom &ref, const thunder::Atom &g)
{
	if (ref.index() != g.index())
		return false;

	if (auto g_global = g.get <thunder::Global> ()) {
		auto ref_global = ref.get <thunder::Global> ();
		// TODO: recursive check if not -max
		return (ref_global->qualifier == g_global->qualifier)
			&& (ref_global->binding == ref_global->binding);
	}

	if (auto g_global = g.get <thunder::TypeField> ()) {
		auto ref_global = ref.get <thunder::TypeField> ();
		return (ref_global->item == g_global->item);
	}

	return false;
}

int ir_op_occurence(const thunder::Scratch &scratch, const thunder::Atom &g)
{
	int count = 0;
	for (size_t i = 0; i < scratch.pointer; i++) {
		thunder::Atom atom = scratch.pool[i];
		if (ir_cmp_op(atom, g))
			count++;
	}

	return count;
}

template <primitive_type T>
void synthesize_layout_io_inner()
{
	thunder::Scratch scratch;

	Emitter::active.push(scratch);
	{
		layout_in <T> lin(0);
		layout_out <T> lout(0);
		lout = lin;
	}
	Emitter::active.pop();

	thunder::Global lin;
	lin.qualifier = thunder::GlobalQualifier::layout_in;
	lin.binding = 0;

	thunder::Global lout;
	lout.qualifier = thunder::GlobalQualifier::layout_out;
	lout.binding = 0;

	thunder::TypeField type_field;
	type_field.item = synthesize_primitive_type <T> ();

	EXPECT_EQ(ir_op_occurence(scratch, lin), 1);
	EXPECT_EQ(ir_op_occurence(scratch, lout), 1);
	EXPECT_EQ(ir_op_occurence(scratch, type_field), 2);
}

TEST(ire_emitter, synthesize_layout_io_int)
{
	synthesize_layout_io_inner <int> ();
}

TEST(ire_emitter, synthesize_layout_io_float)
{
	synthesize_layout_io_inner <float> ();
}

TEST(ire_emitter, synthesize_layout_io_bool)
{
	synthesize_layout_io_inner <bool> ();
}
