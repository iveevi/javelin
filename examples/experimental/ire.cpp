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

// Higher level intermediate representation
namespace jvl::mir {

// TODO: should use some arena allocator for this
// TODO: use uint32_t index and then a global array for the actual data
template <typename T>
struct Ptr : std::shared_ptr <T> {
	Ptr() = default;
	Ptr(const T &t) : std::shared_ptr <T> (std::make_shared <T> (t)) {}

	std::string to_string() const {
		return fmt::format("%{}", (void *) this->get());
	}
};

using Int = int;
using Float = float;
using Bool = bool;
using String = std::string;

struct Molecule;

struct Type;
struct Field : bestd::variant <Ptr <Type>, thunder::PrimitiveType> {
	using bestd::variant <Ptr <Type>, thunder::PrimitiveType>::variant;

	std::string to_string() const;
};

// TODO: use an arena allocator for this
struct Type {
	std::vector <Field> fields;
	std::vector <thunder::QualifierKind> qualifiers;

	std::string to_string() const {
		std::string result;

		for (auto &field : fields)
			result += fmt::format("{} ", field.to_string());

		if (qualifiers.size() > 0)
			result += ": ";

		for (auto &qualifier : qualifiers)
			result += fmt::format("{} ", thunder::tbl_qualifier_kind[qualifier]);

		return result;
	}
};

std::string Field::to_string() const
{
	if (auto type = this->get <Ptr <Type>> ())
		return type.value()->to_string();
	else
		return thunder::tbl_primitive_types[this->as <thunder::PrimitiveType> ()];
}

struct Primitive : bestd::variant <Int, Float, Bool, String> {
	using bestd::variant <Int, Float, Bool, String>::variant;

	std::string to_string() const {
		switch (this->index()) {
		variant_case(Primitive, Int):
			return fmt::format("i32({})", this->as <Int> ());
		variant_case(Primitive, Float):
			return fmt::format("f32({})", this->as <Float> ());
		variant_case(Primitive, Bool):
			return fmt::format("bool({})", this->as <Bool> ());
		variant_case(Primitive, String):
			return fmt::format("string(\"{}\")", this->as <String> ());
		}

		return "not implemented";
	}
};

struct Operation {
	Ptr <Molecule> a;
	Ptr <Molecule> b;
	thunder::OperationCode code;

	std::string to_string() const {
		if (b) {
			return fmt::format("{} {} {}",
				thunder::tbl_operation_code[code],
				a.to_string(),
				b.to_string());
		} else {
			return fmt::format("{} {}",
				thunder::tbl_operation_code[code],
				a.to_string());
		}
	}
};

// TODO: use something else for intrinsic...
struct Intrinsic {
	std::vector <Ptr <Molecule>> args;
	thunder::IntrinsicOperation opn;

	std::string to_string() const {
		std::string result;

		result += fmt::format("{} ", thunder::tbl_intrinsic_operation[opn]);

		for (auto &arg : args)
			result += fmt::format("{} ", arg.to_string());

		return result;
	}
};

struct Construct {
	Ptr <Type> type;
	std::vector <Ptr <Molecule>> args;
	thunder::ConstructorMode mode;

	std::string to_string() const {
		std::string result;

		result += fmt::format("{} new ", type.to_string());
		for (auto &arg : args)
			result += fmt::format("{} ", arg.to_string());

		result += fmt::format("{}", thunder::tbl_constructor_mode[mode]);

		return result;
	}
};

struct Call {
	Int callee;
	std::vector <Ptr <Molecule>> args;
	Ptr <Type> type;

	std::string to_string() const {
		return "not implemented";
	}
};

struct Store {
	Ptr <Molecule> dst;
	Ptr <Molecule> src;

	std::string to_string() const {
		return fmt::format("store {} {}", dst.to_string(), src.to_string());
	}
};

struct Load {
	Ptr <Molecule> src;
	Int idx;

	std::string to_string() const {
		return fmt::format("load {} {}", src.to_string(), idx);
	}
};

struct Index {
	Ptr <Molecule> src;
	Ptr <Molecule> idx;

	std::string to_string() const {
		return "not implemented";
	}
};

struct Block {
	std::vector <Ptr <Molecule>> parameters;
	std::vector <Ptr <Molecule>> body;

	std::string to_string() const;
};

struct Branch {
	Ptr <Molecule> condition;
	Ptr <Block> then;
	Ptr <Block> otherwise;

	std::string to_string() const {
		return "not implemented";
	}
};

struct Phi {
	std::vector <Ptr <Molecule>> values;
	std::vector <Ptr <Block>> blocks;

	std::string to_string() const {
		return "not implemented";
	}
};

struct Return {
	Ptr <Molecule> value;

	std::string to_string() const {
		return fmt::format("return {}", value.to_string());
	}
};

using molecule_base = bestd::variant <
	Type,
	Primitive,
	Operation,
	Intrinsic,
	Construct,
	Call,
	Store,
	Load,
	Index,
	Block,
	Branch,
	Phi,
	Return
>;

struct Molecule : molecule_base {
	using molecule_base::molecule_base;

	std::string to_string() const {
		auto ftn = [](auto &self) -> std::string {
			return self.to_string();
		};

		return std::visit(ftn, *this);
	}
};

std::string Block::to_string() const
{
	std::string result;

	result += "parameters:\n";
	for (auto &parameter : parameters) {
		result += fmt::format("    {} = {}\n",
			parameter.to_string(),
			parameter->to_string());
	}

	result += "body:\n";
	for (auto &molecule : body) {
		result += fmt::format("    {} = {}\n",
			molecule.to_string(),
			molecule->to_string());
	}

	return result;
}

std::string format_as(const Block &block)
{
	return block.to_string();
}

} // namespace jvl::mir

namespace jvl::thunder {

mir::Block convert(const Buffer &buffer)
{
	// Cumulative mapping
	std::map <Index, mir::Ptr <mir::Molecule>> mapping;

	mir::Block block;

	auto list_walk = [&](Index i) {
		std::vector <mir::Ptr <mir::Molecule>> result;

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

			auto &ptr = mapping[qualifier.underlying];
			JVL_ASSERT(ptr->is <mir::Type> (), "expected type");

			auto &type = ptr->as <mir::Type> ();
			type.qualifiers.push_back(qualifier.kind);

			if (qualifier.kind == thunder::parameter) {
				Index which = qualifier.numerical;
				if (which >= block.parameters.size())
					block.parameters.resize(which + 1);

				block.parameters[which] = ptr;
			}

			mapping[i] = ptr;
		} break;

		variant_case(Atom, TypeInformation):
		{
			auto &ti = atom.as <TypeInformation> ();

			if (ti.next == -1 && ti.down == -1) {
				auto field = mir::Field(ti.item);

				auto type = mir::Type();
				type.fields.push_back(field);

				auto ptr = mir::Ptr <mir::Molecule> (type);
				mapping[i] = ptr;
				block.body.push_back(ptr);
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

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
		} break;

		variant_case(Atom, Swizzle):
		{
			auto &swizzle = atom.as <Swizzle> ();
			JVL_ASSERT(mapping.contains(swizzle.src), "expected type");

			auto &opda = mapping[swizzle.src];

			auto p = mir::Operation();
			p.a = opda;
			p.code = OperationCode(int(swizzle.code) + int(thunder::swz_x));

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
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

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
		} break;

		variant_case(Atom, Intrinsic):
		{
			auto &intrinsic = atom.as <Intrinsic> ();

			auto args = list_walk(intrinsic.args);

			auto p = mir::Intrinsic();
			p.args = args;
			p.opn = intrinsic.opn;

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
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
			ctor.type = type->as <mir::Type> ();
			ctor.args = args;
			ctor.mode = construct.mode;

			auto ptr = mir::Ptr <mir::Molecule> (ctor);
			mapping[i] = ptr;
			block.body.push_back(ptr);
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

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
		} break;

		variant_case(Atom, Returns):
		{
			auto &returns = atom.as <Returns> ();
			JVL_ASSERT(mapping.contains(returns.value), "expected type");

			auto value = mapping[returns.value];

			auto p = mir::Return();
			p.value = value;

			auto ptr = mir::Ptr <mir::Molecule> (p);
			mapping[i] = ptr;
			block.body.push_back(ptr);
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
	thunder::optimize(ftn);
	ftn.display_assembly();

	auto mir = convert(ftn);

	fmt::println("{}", mir);
}
