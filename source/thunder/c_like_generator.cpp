#include "thunder/atom.hpp"
#include "thunder/c_like_generator.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder::detail {

MODULE(c-like-generator);

static std::optional <std::string> generate_global_reference(const Qualifier &qualifier)
{
	switch (qualifier.kind) {
	case parameter:
		return fmt::format("_arg{}", qualifier.numerical);

	// GLSL input/output etc. qualifiers
	case layout_in_flat:
	case layout_in_noperspective:
	case layout_in_smooth:
		return fmt::format("_lin{}", qualifier.numerical);
	case layout_out_flat:
	case layout_out_noperspective:
	case layout_out_smooth:
		return fmt::format("_lout{}", qualifier.numerical);

	case push_constant:
		return "_pc";

	case uniform:
		return fmt::format("_uniform{}", qualifier.numerical);

	case storage_buffer:
	case read_only_storage_buffer:
	case write_only_storage_buffer:
		return fmt::format("_buffer{}", qualifier.numerical);

	case shared:
		return fmt::format("_shared{}", qualifier.numerical);

	// GLSL images and samplers
	case isampler_1d:
	case isampler_2d:
	case isampler_3d:
	case usampler_1d:
	case usampler_2d:
	case usampler_3d:
	case sampler_1d:
	case sampler_2d:
	case sampler_3d:
		return fmt::format("_sampler{}", qualifier.numerical);

	// GLSL shader stage intrinsics
	case glsl_intrinsic_gl_FragCoord:
		return "gl_FragCoord";
	case glsl_intrinsic_gl_FragDepth:
		return "gl_FragDepth";
	case glsl_intrinsic_gl_InstanceID:
		return "gl_InstanceID";
	case glsl_intrinsic_gl_InstanceIndex:
		return "gl_InstanceIndex";
	case glsl_intrinsic_gl_VertexID:
		return "gl_VertexID";
	case glsl_intrinsic_gl_VertexIndex:
		return "gl_VertexIndex";
	case glsl_intrinsic_gl_GlobalInvocationID:
		return "gl_GlobalInvocationID";

	case glsl_intrinsic_gl_Position:
		return "gl_Position";

	default:
		break;
	}

	return std::nullopt;
}

static std::string arguments_to_string(const std::vector <std::string> &args)
{
	std::string ret;
	ret += "(";
	for (size_t i = 0; i < args.size(); i++) {
		ret += args[i];
		if (i + 1 < args.size())
			ret += ", ";
	}

	ret += ")";
	return ret;
}

std::string generate_primitive(const Primitive &p)
{
	switch (p.type) {
	case boolean:
		return fmt::format("{}", p.bdata);
	case i32:
		return fmt::format("{}", p.idata);
	case u32:
		return fmt::format("{}", p.udata);
	case f32:
		return fmt::format("{}", p.fdata);
	default:
		break;
	}

	JVL_ABORT("unsupported primitive: {}", p);
}

std::string generate_operation(OperationCode code, const std::string &a, const std::string &b)
{
	// Binary operator strings
	static const wrapped::hash_table <OperationCode, const char *> operators {
		{ addition,		"+" },
		{ subtraction,		"-" },
		{ multiplication,	"*" },
		{ division,		"/" },

		{ modulus,		"%" },

		{ bit_shift_left,	"<<" },
		{ bit_shift_right,	">>" },

		{ bit_and,		"&" },
		{ bit_or,		"|" },
		{ bit_xor,		"^" },

		{ cmp_ge,		">" },
		{ cmp_geq,		">=" },
		{ cmp_le,		"<" },
		{ cmp_leq,		"<=" },
		{ equals,		"==" },
		{ not_equals,		"!=" },
	};

	// Handle the special cases
	if (code == unary_negation)
		return fmt::format("-({})", a);
	if (code == bool_not)
		return fmt::format("!({})", a);

	// Should be left with purely binary operations
	JVL_ASSERT(operators.contains(code),
		"no operator symbol found for $({})",
		tbl_operation_code[code]);

	const char *const op = operators.at(code);

	return fmt::format("({} {} {})", a, op, b);
}

c_like_generator_t::c_like_generator_t(const auxiliary_block_t &body)
	: auxiliary_block_t(body), indentation(1) {}

void c_like_generator_t::finish(const std::string &s, bool semicolon)
{
	source += std::string(indentation << 2, ' ') + s + (semicolon ? ";" : "") + "\n";
}

void c_like_generator_t::declare(index_t index)
{
	auto t = type_to_string(types[index]);
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {}{}", t.pre, var, t.post);
	local_variables[index] = var;
	finish(stmt);
}

void c_like_generator_t::define(index_t index, const std::string &v)
{
	auto t = type_to_string(types[index]);
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {}{} = {}", t.pre, var, t.post, v);
	local_variables[index] = var;
	finish(stmt);
}

void c_like_generator_t::assign(int index, const std::string &v)
{
	std::string stmt = fmt::format("{} = {}", reference(index), v);
	finish(stmt);
}

std::string c_like_generator_t::reference(index_t index) const
{
	JVL_ASSERT(index != -1, "invalid index passed to ref");

	if (local_variables.count(index))
		return local_variables.at(index);

	const Atom &atom = atoms[index];

	switch (atom.index()) {

	case Atom::type_index <Qualifier> ():
	{
		auto ref = generate_global_reference(atom.as <Qualifier> ());
		if (ref)
			return ref.value();
	} break;

	case Atom::type_index <Construct> ():
	{
		auto &constructor = atom.as <Construct> ();
		if (constructor.mode == transient)
			return inlined(constructor.type);
	} break;

	case Atom::type_index <Load> ():
	{
		auto &load = atom.as <Load> ();

		std::string accessor;
		if (load.idx != -1)
			accessor = fmt::format(".f{}", load.idx);

		return reference(load.src) + accessor;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swizzle = atom.as <Swizzle> ();
		std::string accessor = tbl_swizzle_code[swizzle.code];
		return reference(swizzle.src) + "." + accessor;
	}

	case Atom::type_index <ArrayAccess> ():
	{
		auto &access = atom.as <ArrayAccess> ();
		return fmt::format("{}[{}]", inlined(access.src), inlined(access.loc));
	}

	default:
		break;
	}

	// TODO: Could be problematic, its not an actual storage location
	return inlined(index);
}

std::string c_like_generator_t::inlined(index_t index) const
{
	JVL_ASSERT(index != -1, "invalid index passed to inlined");

	if (local_variables.count(index))
		return local_variables.at(index);

	const Atom &atom = atoms[index];

	switch (atom.index()) {

	case Atom::type_index <Qualifier> ():
	{
		auto ref = generate_global_reference(atom.as <Qualifier> ());
		if (ref)
			return ref.value();
	} break;

	case Atom::type_index <Primitive> ():
		return generate_primitive(atom.as <Primitive> ());

	case Atom::type_index <Operation> ():
	{
		auto &operation = atom.as <Operation> ();
		std::string a = inlined(operation.a);
		std::string b = (operation.b == -1) ? "" : inlined(operation.b);
		return generate_operation(operation.code, a, b);
	}

	case Atom::type_index <Intrinsic> ():
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = arguments(intrinsic.args);
		return tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
	}

	case Atom::type_index <Construct> ():
	{
		auto &constructor = atom.as <Construct> ();
		if (constructor.mode == transient)
			return inlined(constructor.type);

		auto t = type_to_string(types[index]);
		if (constructor.args != -1) {
			auto args = arguments(constructor.args);
			return t.pre + t.post + arguments_to_string(args);
		}

		return t.pre + t.post + "()";
	}

	case Atom::type_index <Call> ():
	{
		auto &call = atom.as <Call> ();

		TrackedBuffer *cbl = TrackedBuffer::search_tracked(call.cid);
		std::string args;
		if (call.args != -1)
			args = arguments_to_string(arguments(call.args));

		return fmt::format("{}{}", cbl->name, args);
	}

	case Atom::type_index <Load> ():
	case Atom::type_index <Swizzle> ():
	case Atom::type_index <ArrayAccess> ():
		return reference(index);

	default:
		break;
	}

	JVL_ABORT("failed to inline atom: {} (@{})", atom, index);
}

std::vector <std::string> c_like_generator_t::arguments(index_t start) const
{
	std::vector <std::string> args;

	int l = start;
	while (l != -1) {
		Atom h = atoms[l];
		if (!h.is <List> ()) {
			fmt::println("unexpected atom in arglist: {}", h.to_string());
			fmt::print("\n");
			abort();
		}

		List list = h.as <List> ();
		if (list.item == -1) {
			fmt::println("invalid index found in list item");
			abort();
		}

		args.push_back(inlined(list.item));

		l = list.next;
	}

	return args;
}

c_like_generator_t::type_string c_like_generator_t::type_to_string(const QualifiedType &qt) const
{
	switch (qt.index()) {

	variant_case(QualifiedType, NilType):
		return { "void", "" };

	variant_case(QualifiedType, ArrayType):
	{
		auto &at = qt.as <ArrayType> ();
		auto base = type_to_string(at.base());

		std::string array = fmt::format("[{}]", at.size);

		// Unsize arrays
		if (at.size < 0)
			array = "[]";

		return {
			.pre = base.pre,
			.post = fmt::format("{}{}", base.post, array)
		};
	}

	variant_case(QualifiedType, PlainDataType):
	{
		auto &pd = qt.as <PlainDataType> ();
		if (auto p = pd.get <PrimitiveType> ())
			return { .pre = tbl_primitive_types[*p], .post = "" };

		index_t concrete = pd.as <index_t> ();
		if (struct_names.contains(concrete))
			return { .pre = struct_names.at(concrete), .post = "" };

		return type_to_string(types[concrete]);
	}

	variant_case(QualifiedType, InArgType):
	{
		auto &in = qt.as <InArgType> ();
		auto base = static_cast <PlainDataType> (in);
		auto info = type_to_string(base);

		return {
			.pre = "in " + info.pre,
			.post = info.post
		};
	}

	variant_case(QualifiedType, OutArgType):
	{
		auto &out = qt.as <OutArgType> ();
		auto base = static_cast <PlainDataType> (out);
		auto info = type_to_string(base);

		return {
			.pre = "out " + info.pre,
			.post = info.post
		};
	}

	variant_case(QualifiedType, InOutArgType):
	{
		auto &inout = qt.as <InOutArgType> ();
		auto base = static_cast <PlainDataType> (inout);
		auto info = type_to_string(base);

		return {
			.pre = "inout " + info.pre,
			.post = info.post
		};
	}

	default:
		break;
	}

	dump();

	JVL_ABORT("failed to resolve type name for {}", qt);
}

// Generators for each kind of instruction
template <>
void c_like_generator_t::generate(const Qualifier &, index_t)
{
	// Nothing to do here, linkage should have taken
	// care of generating the necessary structs; their
	// assigned names should all be in struct_names
}

template <>
void c_like_generator_t::generate(const TypeInformation &, index_t)
{
	// Same idea here; all globals should be
	// instatiated from the linker side
}

template <>
void c_like_generator_t::generate(const Primitive &primitive, index_t index)
{
	define(index, generate_primitive(primitive));
}

template <>
void c_like_generator_t::generate(const Swizzle &swizzle, index_t index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Operation &operation, index_t index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Intrinsic &intrinsic, index_t index)
{
	// Special cases
	if (intrinsic.opn == thunder::set_local_size)
		return;

	// Keyword intrinsic
	if (intrinsic.args == -1)
		return finish(tbl_intrinsic_operation[intrinsic.opn]);

	auto &qt = types[index];
	if (qt.is <NilType> ()) {
		// Void return type, so no assignment
		auto args = arguments(intrinsic.args);
		std::string v = tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
		return finish(v);
	} else {
		auto args = arguments(intrinsic.args);
		auto intr = tbl_intrinsic_operation[intrinsic.opn];
		return define(index, intr + arguments_to_string(args));
	}
}

template <>
void c_like_generator_t::generate(const Construct &construct, index_t index)
{
	if (construct.mode == transient)
		return;

	if (construct.args == -1)
		return declare(index);

	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Call &call, index_t index)
{
	TrackedBuffer *cbl = TrackedBuffer::search_tracked(call.cid);
	std::string args = "()";
	if (call.args != -1)
		args = arguments_to_string(arguments(call.args));

	define(index, cbl->name + args);
}

template <>
void c_like_generator_t::generate(const Store &store, index_t)
{
	assign(store.dst, inlined(store.src));
}

template <>
void c_like_generator_t::generate(const Load &load, index_t index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const ArrayAccess &access, index_t index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Branch &branch, index_t)
{
	switch (branch.kind) {

	case control_flow_end:
		indentation--;
		finish("}", false);
		return;

	case conditional_if:
		finish(fmt::format("if ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case loop_while:
		finish(fmt::format("while ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case conditional_else_if:
		indentation--;
		finish(fmt::format("}} else if ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case conditional_else:
		indentation--;
		finish("} else {", false);
		indentation++;
		return;

	case control_flow_skip:
		return finish("continue", true);

	case control_flow_stop:
		return finish("break", true);

	default:
		break;
	}

	JVL_ABORT("failed to generate branch: {}", branch);
}

template <>
void c_like_generator_t::generate(const Returns &returns, index_t)
{
	if (returns.value >= 0)
		finish("return " + inlined(returns.value));
	else
		finish("return");
}

// Per-atom generator
void c_like_generator_t::generate(index_t i)
{
	auto ftn = [&](auto atom) { return generate(atom, i); };
	return std::visit(ftn, atoms[i]);
}

// General generator
std::string c_like_generator_t::generate()
{
	for (size_t i = 0; i < pointer; i++) {
		if (synthesized.count(i)) {
			// fmt::println("  generating: {}", atoms[i]);
			generate(i);
		}
	}

	return source;
}

} // namespace jvl::thunder::detail
