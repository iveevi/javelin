// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include <thunder/mir/memory.hpp>
#include <thunder/mir/molecule.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: get line numbers for each invocation if possible?
// using source location if available

namespace jvl::thunder {

mir::Block convert(const Buffer &buffer)
{
	// Cumulative mapping
	std::map <Index, mir::Ref <mir::Molecule>> mapping;
	std::map <Index, Index> circuit;

	mir::Block block;

	auto list_walk = [&](Index i) -> mir::Seq <mir::Ref <mir::Molecule>> {
		std::vector <mir::Ref <mir::Molecule>> result;

		while (i != -1) {
			auto &atom = buffer.atoms[i];
			JVL_ASSERT(atom.is <List> (), "expected list");

			auto &list = atom.as <List> ();
			JVL_ASSERT(mapping.contains(list.item), "expected type");

			auto &ptr = mapping[list.item];

			result.push_back(ptr);

			i = list.next;
		}

		return result;
	};

	for (Index i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];

		switch (atom.index()) {

		variant_case(Atom, Qualifier):
		{
			auto &qualifier = atom.as <Qualifier> ();
			JVL_ASSERT(mapping.contains(qualifier.underlying), "expected type");

			auto ptr = mapping[qualifier.underlying];
			JVL_ASSERT(ptr->is <mir::Type> (), "expected type");

			auto type = ptr->as <mir::Type> ();
			type.qualifiers.add(qualifier.kind);

			ptr = mir::Ref <mir::Molecule> (type);

			// TODO: hosting to higher scopes...
			// if (qualifier.kind == thunder::parameter) {
			// 	Index which = qualifier.numerical;
			// 	if (which >= block.parameters.size())
			// 		block.parameters.resize(which + 1);

			// 	block.parameters[which] = ptr;
			// }

			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, TypeInformation):
		{
			auto &ti = atom.as <TypeInformation> ();

			if (ti.next == -1 && ti.down == -1) {
				auto field = mir::Field(ti.item);

				auto type = mir::Type();
				type.fields.add(field);

				auto ptr = mir::Ref <mir::Molecule> (type);
				mapping[i] = ptr;
				block.body.add(ptr);
			} else {
				JVL_ABORT("not implemented");
			}
		} break;

		variant_case(Atom, Primitive):
		{
			auto &primitive = atom.as <Primitive> ();

			auto p = mir::Primitive();

			switch (primitive.type) {
			case thunder::boolean:
				p = mir::Bool(primitive.bdata);
				break;
			case thunder::i32:
				p = mir::Int(primitive.idata);
				break;
			case thunder::u32:
				p = mir::Int(primitive.udata);
				break;
			case thunder::f32:
				p = mir::Float(primitive.fdata);
				break;
			default:
				JVL_ABORT("not implemented");
			}

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, Swizzle):
		{
			auto &swizzle = atom.as <Swizzle> ();
			JVL_ASSERT(mapping.contains(swizzle.src), "expected type");

			auto &opda = mapping[swizzle.src];

			auto p = mir::Operation();
			p.a = opda;
			p.code = OperationCode(int(swizzle.code) + int(thunder::swz_x));

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, Operation):
		{
			auto &operation = atom.as <Operation> ();
			JVL_ASSERT(mapping.contains(operation.a), "expected type");

			auto &opda = mapping[operation.a];

			auto p = mir::Operation();
			p.a = opda;

			if (operation.b) {
				JVL_ASSERT(mapping.contains(operation.b), "expected type");
				p.b = mapping[operation.b];
			}

			p.code = operation.code;

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, Intrinsic):
		{
			auto &intrinsic = atom.as <Intrinsic> ();

			auto args = list_walk(intrinsic.args);

			auto p = mir::Intrinsic();
			p.args = args;
			p.opn = intrinsic.opn;

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, List):
			// Do nothing...
			break;

		variant_case(Atom, Construct):
		{
			auto &construct = atom.as <Construct> ();
			JVL_ASSERT(mapping.contains(construct.type), "expected type");

			auto type = mapping[construct.type];
			JVL_ASSERT(type->is <mir::Type> (), "expected type");

			auto args = list_walk(construct.args);

			auto ctor = mir::Construct();
			ctor.type = type.index;
			ctor.args = args;
			ctor.mode = construct.mode;

			auto ptr = mir::Ref <mir::Molecule> (ctor);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, Store):
		{
			auto &store = atom.as <Store> ();
			JVL_ASSERT(mapping.contains(store.src), "expected type");
			JVL_ASSERT(mapping.contains(store.dst), "expected type");

			auto src = mapping[store.src];
			auto dst = mapping[store.dst];

			auto p = mir::Store();
			p.src = src;
			p.dst = dst;

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		variant_case(Atom, Return):
		{
			auto &returns = atom.as <Return> ();
			JVL_ASSERT(mapping.contains(returns.value), "expected type");

			auto value = mapping[returns.value];

			auto p = mir::Return();
			p.value = value;

			auto ptr = mir::Ref <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.add(ptr);
		} break;

		default:
			JVL_WARNING("conversion not implemented for atom: {}", atom.to_assembly_string());
		}
	}

	return block;
}

} // namespace jvl::thunder

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

	auto mir = convert(ftn);

	fmt::println("{}", mir);
}
