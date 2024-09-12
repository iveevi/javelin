#include <gtest/gtest.h>

#include "ire/core.hpp"
#include "ire/type_synthesis.hpp"
#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/enumerations.hpp"

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
		index_t in = em.emit_qualifier(type_in, 0, thunder::layout_in);
		index_t ld = em.emit_load(in, -1);
		
		// Duplicate type is expected to be generated
		index_t type_out = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		index_t out = em.emit_qualifier(type_out, 0, thunder::layout_out);
		index_t st = em.emit_store(out, ld, false);
	}
	em.pop();

	// Generate code using the IRE
	thunder::Buffer ire_buffer;

	em.push(ire_buffer);
	{
		layout_in <T> lin(0);
		layout_out <T> lout(0);
		lout = lin;
	}
	em.pop();

	ASSERT_TRUE(check_contents(ref_buffer, ire_buffer));
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
