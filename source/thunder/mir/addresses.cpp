#include "thunder/mir/transformations.hpp"

namespace jvl::thunder::mir {

Set addresses(const Type &type)
{
	Set result;

	if (type.is <Aggregate> ()) {
		auto &aggr = type.as <Aggregate> ();

		for (auto &field : aggr.fields)
			result.insert(field.index);
	}

	return result;
}

Set addresses(const Construct &ctor)
{
	Set result;

	result.insert(ctor.type.index);

	for (auto &arg : ctor.args)
		result.insert(arg.index);

	return result;
}

Set addresses(const Operation &opn)
{
	Set result;
	if (opn.a)
		result.insert(opn.a.index);
	if (opn.b)
		result.insert(opn.b.index);

	return result;
}

Set addresses(const Intrinsic &intr)
{
	Set result;
	for (auto &p : intr.args)
		result.insert(p.index);

	return result;
}

Set addresses(const Store &store)
{
	return { store.src.index, store.dst.index };
}

Set addresses(const Storage &storage)
{
	return { storage.type.index };
}

Set addresses(const Return &returns)
{
	return { returns.value.index };
}

} // namespace jvl::thunder::mir