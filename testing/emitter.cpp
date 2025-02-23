#include <gtest/gtest.h>

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(test-emitter);

// Checking that certain operations appear in IR
bool check_contents(const thunder::TrackedBuffer &ref, const thunder::TrackedBuffer &given)
{
	if (ref.pointer != given.pointer) {
		ref.dump();
		given.dump();
		return false;
	}

	for (size_t i = 0; i < ref.pointer; i++) {
		if (ref[i] != given[i]) {
			ref.dump();
			given.dump();
			JVL_ERROR("generate buffer differs @{}:\n{}\nVersus:\n{}",
				i, ref[i], given[i]);
			return false;
		}
	}

	return true;
}

template <native T>
void synthesize_layout_io_inner()
{
	auto &em = Emitter::active;

	// Generate code manually as a reference
	thunder::TrackedBuffer ref_buffer;
	ref_buffer.name = "Reference";

	thunder::PrimitiveType p = thunder::boolean;
	if constexpr (std::same_as <T, int32_t>)
		p = thunder::i32;
	if constexpr (std::same_as <T, uint32_t>)
		p = thunder::u32;
	if constexpr (std::same_as <T, float>)
		p = thunder::f32;

	em.push(ref_buffer);
	{
		using thunder::Index;

		em.emit_primitive(T());
		auto type_in = em.emit_type_information(-1, -1, p);
		auto in_ql = em.emit_qualifier(type_in, 0, thunder::layout_in_smooth);
		auto in = em.emit_construct(in_ql, -1, jvl::thunder::transient);
		
		// Duplicate type is expected to be generated
		em.emit_primitive(T());
		auto type_out = em.emit_type_information(-1, -1, p);
		auto out_ql = em.emit_qualifier(type_out, 0, thunder::layout_out_smooth);
		auto out = em.emit_construct(out_ql, -1, jvl::thunder::transient);
		em.emit_store(out, in);
	}
	em.pop();

	// Generate code using the IRE
	thunder::TrackedBuffer buffer;
	buffer.name = "Generated";

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
	synthesize_layout_io_inner <int32_t> ();
}

TEST(emitter, synthesize_layout_io_uint)
{
	synthesize_layout_io_inner <uint32_t> ();
}

TEST(emitter, synthesize_layout_io_float)
{
	synthesize_layout_io_inner <float> ();
}

TEST(emitter, synthesize_layout_io_bool)
{
	synthesize_layout_io_inner <bool> ();
}

// TODO: more tests...