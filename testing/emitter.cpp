#include <gtest/gtest.h>

#include <ire.hpp>

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
		using thunder::Index;

		auto type_in = native_t <T> ::type();
		auto in_ql = em.emit_qualifier(type_in, 0, thunder::layout_in_smooth);
		auto in = em.emit_construct(in_ql, -1, jvl::thunder::transient);
		
		// Duplicate type is expected to be generated
		auto type_out = native_t <T> ::type();
		auto out_ql = em.emit_qualifier(type_out, 0, thunder::layout_out_smooth);
		auto out = em.emit_construct(out_ql, -1, jvl::thunder::transient);
		em.emit_store(out, in);
	}
	em.pop();

	// Generate code using the IRE
	thunder::Buffer buffer;

	em.push(buffer);
	{
		layout_in <native_t <T>> lin(0);
		layout_out <native_t <T>> lout(0);
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
