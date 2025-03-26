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
#include <thunder/mir/transformations.hpp>

#include "common/io.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(mir);

namespace jvl::thunder::mir {

/////////////////////////
// Replacing addresses //
/////////////////////////

template <typename T>
requires bestd::is_variant_component <molecule_base, T>
void readdress(T &, Index a, Index b) {}

void readdress(Operation &opn, Index a, Index b)
{
	if (opn.a.index == a)
		opn.a = b;
	if (opn.b.index == a)
		opn.b = b;
}

void readdress(Molecule &mole, Index a, Index b)
{
	auto ftn = [&](auto &x) { return readdress(x, a, b); };
	return std::visit(ftn, mole);
}

// Usage and users graphs
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

}

Block legalize(const Block &block)
{

}

struct OptimizationPassResult {
	Block block;
	bool changed;
};

OptimizationPassResult optimize_dead_code_elimination_iteration(const Block &block)
{
	Linear instructions;

	auto users = mole_users(block);

	for (auto &r : block.body) {
		bool exempt = false;
		exempt |= r->is <Store> ();
		exempt |= r->is <Return> ();

		bool empty = !users.contains(r.idx()) || users.at(r.idx()).empty();
		
		if (exempt || !empty)
			instructions.emplace_back(r);
	}

	return OptimizationPassResult {
		.block = Block(instructions),
		.changed = block.body.count != instructions.size()
	};
}

Block optimize_dead_code_elimination(const Block &block)
{
	OptimizationPassResult pass;
	pass.block = block;

	do {
		pass = optimize_dead_code_elimination_iteration(pass.block);
		fmt::println("DEC pass... (to {} moles)", pass.block.body.count);
	} while (pass.changed);

	return pass.block;
}

// TODO: lvalue classification...
// TODO: inout legalization...
// TODO: deduplication...
// TODO: then casting elision...

} // namespace jvl::thunder::mir

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

	mir = thunder::mir::legalize_storage(mir);
	fmt::println("{}", mir);
	mir.graphviz("mir-legalized.dot");
	
	mir = thunder::mir::optimize_dead_code_elimination(mir);
	fmt::println("{}", mir);
	mir.graphviz("mir-dec.dot");
}
