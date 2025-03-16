// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <bestd/tree.hpp>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

#include "common/io.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(mir);

namespace jvl::thunder::mir {

using Set = std::set <Index>;

template <typename T>
requires bestd::is_variant_component <molecule_base, T>
Set addresses(const T &)
{
	return {};
}

Set addresses(const Type &type)
{
	Set result;

	if (type.is <Aggregate> ()) {
		auto &aggr = type.as <Aggregate> ();

		size_t i = 0;
		for (auto &field : aggr.fields)
			result.insert(field.index);
	}

	return result;
}

Set addresses(const Construct &ctor)
{
	Set result;

	result.insert(ctor.type.index);

	size_t i = 0;
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

Set addresses(const Return &returns)
{
	return { returns.value.index };
}

bestd::tree <Index, Set> mole_usage(const Block &block)
{
	bestd::tree <Index, Set> graph;

	for (auto &p : block.body) {
		auto ftn = [&](auto x) { return addresses(x); };
		graph[p.idx()] = std::visit(ftn, *p);
	}

	return graph;
}

bestd::tree <Index, Set> mole_users(const Block &block)
{
	bestd::tree <Index, Set> graph;

	for (auto &p : block.body) {
		auto ftn = [&](auto x) { return addresses(x); };
		auto set = std::visit(ftn, *p);
		for (auto v : set)
			graph[v].insert(p.index);
	}

	return graph;
}

using Linear = std::vector <Ref <Molecule>>;

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

			fmt::println("users of store destination @{}: {}", store.dst.idx(), *store.dst);
			auto set = users[store.dst.idx()];
			for (auto v : set) {
				// TODO: ref_load(...)
				fmt::println("\t%{}: {}", v.value, *(Ref <Molecule> (v)));
				// v.get() = storage.idx();
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

// TODO: type deduplication...
// TODO: then casting elision...

} // namespace jvl::thunder::mir

// TODO: dot generator...
// TODO: glsl function body generator...

auto ftn = procedure("area") << [](vec2 x, vec2 y)
{
	return abs(cross(x, y));
};

int main()
{
	ftn.display_assembly();
	link(ftn).write_assembly("ire.jvl.asm");

	// thunder::optimize_dead_code_elimination(ftn);
	thunder::optimize_deduplicate(ftn);
	ftn.display_assembly();

	auto mir = ftn.lower_to_mir();

	fmt::println("{}", mir);

	mir.graphviz("mir.dot");

	auto users = thunder::mir::mole_users(mir);
	
	fmt::println("users graph:");
	for (auto &[k, v] : users) {
		fmt::print("\t{} -> ", k.value);
		for (auto &vv : v)
			fmt::print("{} ", vv.value);
		fmt::print("\n");
	}

	auto usage = thunder::mir::mole_usage(mir);

	fmt::println("usage graph:");
	for (auto &[k, v] : usage) {
		fmt::print("\t@{} ({}) -> ", k.value, *(thunder::mir::Ref <thunder::mir::Molecule> (k)));
		for (auto &vv : v)
			fmt::print("{} ", vv.value);
		fmt::print("\n");
	}

	mir = thunder::mir::legalize_storage(mir);

	fmt::println("{}", mir);

	mir.graphviz("mir-legalized.dot");
}
