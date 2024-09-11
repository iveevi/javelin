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

template <primitive_type T>
void synthesize_layout_io_inner()
{
	auto &em = Emitter::active;

	// Generate code manually as a reference
	thunder::Buffer ref_buffer;

	em.push(ref_buffer);
	{
		using thunder::index_t;

		// TODO: per instruction methods from Emitter
		// em.emit_type_information(...)
		// em.emit_qualifier(...)
		// em.emit_load(...)
		// em.emit_store(...)
		// ... etc
		thunder::TypeInformation type_info;
		type_info.item = synthesize_primitive_type <T> ();

		index_t type = em.emit(type_info);

		thunder::Qualifier input_qualifier;
		input_qualifier.underlying = type;
		input_qualifier.numerical = 0;
		input_qualifier.kind = thunder::layout_in;

		index_t in = em.emit(input_qualifier);

		thunder::Load load;
		load.src = in;

		index_t ld = em.emit(load);
		
		// Duplicate type is expected to be generated
		type = em.emit(type_info);
		
		thunder::Qualifier output_qualifier;
		output_qualifier.underlying = type;
		output_qualifier.numerical = 0;
		output_qualifier.kind = thunder::layout_out;

		index_t out = em.emit(output_qualifier);

		thunder::Store store;
		store.src = ld;
		store.dst = out;

		em.emit(store);
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
