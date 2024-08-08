#include <gtest/gtest.h>

#include "ire/core.hpp"

using namespace jvl::ire;

// Checking that certain operations appear in IR
bool ir_cmp_op(const op::General &ref, const op::General &g)
{
	if (ref.index() != g.index())
		return false;

	if (auto g_global = g.get <op::Global> ()) {
		auto ref_global = ref.get <op::Global> ();
		// TODO: recursive check if not -max
		return (ref_global->qualifier == g_global->qualifier)
			&& (ref_global->binding == ref_global->binding);
	}

	if (auto g_global = g.get <op::TypeField> ()) {
		auto ref_global = ref.get <op::TypeField> ();
		return (ref_global->item == g_global->item);
	}

	return false;
}

bool ir_check_op_occurence(const op::General &g, int occurence = 1)
{
	int count = 0;
	for (size_t i = 0; i < Emitter::active.pointer; i++) {
		op::General g_ref = Emitter::active.pool[i];
		if (ir_cmp_op(g_ref, g))
			count++;
	}

	return (count == occurence);
}

TEST(ire_synthesis, synthesize_empty)
{
	Emitter::active.clear();

	auto shader = []() {
		layout_in <float, 0> lin;
	};

	shader();

	ASSERT_EQ(Emitter::active.pointer, 0);
}

// TEST(ire_synthesis, synthesize_layout_io)

template <primitive_type T>
void synthesize_layout_io_inner()
{
	Emitter::active.clear();

	auto shader = []() {
		layout_in <T, 0> lin;
		layout_out <T, 0> lout;
		lout = lin;
	};

	shader();

	op::Global lin;
	lin.qualifier = op::Global::layout_in;
	lin.binding = 0;

	op::Global lout;
	lout.qualifier = op::Global::layout_out;
	lout.binding = 0;

	op::TypeField type_field;
	type_field.item = synthesize_primitive_type <T> ();

	EXPECT_TRUE(ir_check_op_occurence(lin));
	EXPECT_TRUE(ir_check_op_occurence(lout));
	EXPECT_TRUE(ir_check_op_occurence(type_field, 1));
}

TEST(ire_synthesis, synthesize_layout_io_int)
{
	synthesize_layout_io_inner <int> ();
}

TEST(ire_synthesis, synthesize_layout_io_float)
{
	synthesize_layout_io_inner <float> ();
}

TEST(ire_synthesis, synthesize_layout_io_bool)
{
	synthesize_layout_io_inner <bool> ();
}
