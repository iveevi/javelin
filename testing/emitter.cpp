#include <gtest/gtest.h>

#include <ire/core.hpp>

using namespace jvl;
using namespace jvl::ire;

// Checking that certain operations appear in IR
bool check_contents(const thunder::Buffer &ref, const thunder::Buffer &given)
{
	if (ref.pointer != given.pointer)
		return false;

	for (size_t i = 0; i < ref.pointer; i++) {
		if (ref[i] != given[i])
			return false;
	}

	return true;
}

template <native T>
void synthesize_layout_io_inner()
{
	auto &em = Emitter::active;

	// Generate code manually as a reference
	thunder::Buffer ref_buffer;

	em.push(ref_buffer);
	{
		using thunder::index_t;

		index_t type_in = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		index_t in_ql = em.emit_qualifier(type_in, 0, thunder::layout_in_smooth);
		index_t in = em.emit_construct(in_ql, -1, jvl::thunder::transient);
		
		// Duplicate type is expected to be generated
		index_t type_out = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		index_t out_ql = em.emit_qualifier(type_out, 0, thunder::layout_out_smooth);
		index_t out = em.emit_construct(out_ql, -1, jvl::thunder::transient);
		em.emit_store(out, in);
	}
	em.pop();

	// Generate code using the IRE
	thunder::Buffer buffer;

	em.push(buffer);
	{
		layout_in <T> lin(0);
		layout_out <T> lout(0);
		lout = lin;
	}
	em.pop();

	ASSERT_TRUE(check_contents(ref_buffer, buffer));
}

TEST(emitter, synthesize_layout_io_int)
{
	synthesize_layout_io_inner <int> ();
}

TEST(emitter, synthesize_layout_io_float)
{
	synthesize_layout_io_inner <float> ();
}

TEST(emitter, synthesize_layout_io_bool)
{
	synthesize_layout_io_inner <bool> ();
}
