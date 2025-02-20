#include "ire/native.hpp"

namespace jvl::ire {

///////////////////////////////////////
// Translating native C++ primitives //
///////////////////////////////////////

thunder::Index translate_primitive(bool b)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::boolean;
	p.bdata = b;

	return em.emit(p);
}

thunder::Index translate_primitive(int32_t i)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::i32;
	p.idata = i;

	return em.emit(p);
}

thunder::Index translate_primitive(uint32_t i)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::u32;
	p.udata = i;

	return em.emit(p);
}

thunder::Index translate_primitive(uint64_t i)
{
	auto &em = Emitter::active;

	em.emit_qualifier(-1, -1, thunder::QualifierKind::uint64);

	auto high = translate_primitive(uint32_t(i >> 32));
	auto hlist = em.emit_list(high);
	auto h64 = em.emit_intrinsic(hlist, thunder::IntrinsicOperation::cast_to_uint64);

	auto low = translate_primitive(uint32_t(i & 0xFFFFFFFF));
	auto llist = em.emit_list(low);
	auto l64 = em.emit_intrinsic(llist, thunder::IntrinsicOperation::cast_to_uint64);

	auto shamt = translate_primitive(32u);
	auto shifted = em.emit_operation(h64, shamt, thunder::OperationCode::bit_shift_left);
	auto combined = em.emit_operation(shifted, l64, thunder::OperationCode::bit_or);

	return combined;
}

thunder::Index translate_primitive(float f)
{
	auto &em = Emitter::active;

	thunder::Primitive p;
	p.type = thunder::f32;
	p.fdata = f;

	return em.emit(p);
}

} // namespace jvl::ire