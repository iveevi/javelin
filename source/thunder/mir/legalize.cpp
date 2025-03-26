#include "common/logging.hpp"

#include "thunder/mir/transformations.hpp"

MODULE(legalize);

namespace jvl::thunder::mir {

template <typename T>
Ref <Type> mole_type(Linear &, const T &t)
{
	JVL_ABORT("mole_type not implemented for this molecule: {}", Molecule(t));
}

Ref <Type> mole_type(Linear &linear, const Primitive &p)
{
	PrimitiveType ptype = i32;

	switch (p.index()) {
	variant_case(Primitive, Float):
		ptype = f32;
		break;
	default:
		JVL_ABORT("mole_type not implemented for this molecule: {}", Molecule(p));
	}

	auto type = Ref <Type> (Type(ptype));
	linear.push_back(type.promote());
	return type;
}

Ref <Type> mole_type(Linear &linear, const Molecule &mole)
{
	auto ftn = [&](auto x) { return mole_type(linear, x); };
	return std::visit(ftn, mole);
}

Block legalize_storage(const Block &block)
{
	Linear instructions;

	auto users = mole_users(block);

	bestd::tree <Index::index_type, size_t> map;

	for (auto &r : block.body) {
		auto newr = r;
		if (r->is <Store> ()) {
			fmt::println("store @{}: {}", r.idx(), *r);

			auto &store = r->as <Store> ();
			auto type = mole_type(instructions, *store.dst);
			
			auto storage = Ref <Storage> (Storage(type));
			instructions.push_back(storage.promote());

			auto set = users[store.dst.idx()];
			for (auto v : set) {
				auto &mole = *(Ref <Molecule> (v));
				// TODO: ref_load(...)
				readdress(mole, store.dst.index, storage.index);
			}

			auto new_store = store;
			new_store.dst = storage.promote();
			// TODO: ref_new(T(args...));
			newr = Ref <Molecule> (new_store);
		}

		size_t index = instructions.size();
		map.emplace(r.idx(), index);
		instructions.push_back(newr);
	}

	return Block(instructions);
}

Block legalize_calls(const Block &block)
{
	return Block();
}

Block legalize(const Block &block)
{
	return Block();
}

} // namespace jvl::thunder::mir