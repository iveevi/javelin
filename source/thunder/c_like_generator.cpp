#include "thunder/generators.hpp"
#include "ire/callable.hpp"

namespace jvl::thunder::detail {

MODULE(c-like-generator);

// TODO: pass options to the body_t type
static std::string generate_global_reference(const Qualifier &global)
{
	switch (global.kind) {
	case QualifierKind::layout_in:
		return fmt::format("_lin{}", global.numerical);
	case QualifierKind::layout_out:
		return fmt::format("_lout{}", global.numerical);
	case QualifierKind::parameter:
		return fmt::format("_arg{}", global.numerical);
	case QualifierKind::push_constant:
		return "_pc";
	case QualifierKind::glsl_vertex_intrinsic_gl_Position:
		return "gl_Position";
	default:
		break;
	}

	return "<glo:?>";
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
	case i32:
		return fmt::format("{}", p.idata);
	case u32:
		return fmt::format("{}", p.udata);
	case f32:
		return fmt::format("{}", p.fdata);
	default:
		break;
	}

	return "<prim:?>";
}

std::string generate_operation(OperationCode code, const std::vector <std::string> &args)
{
	// Binary operator strings
	static const wrapped::hash_table <OperationCode, const char *> operators {
		{ addition, "+" },
		{ subtraction, "-" },
		{ multiplication, "*" },
		{ division, "/" },
		
		{ bit_shift_left, "<<" },
		{ bit_shift_right, ">>" },
		
		{ bit_and, "&" },
		{ bit_or, "|" },
		{ bit_xor, "^" },
		
		{ cmp_ge, ">" },
		{ cmp_geq, ">=" },
		{ cmp_le, "<" },
		{ cmp_leq, "<=" },
		{ equals, "==" },
		{ not_equals, "!=" },
	};

	// Handle the special cases
	if (code == unary_negation) {
		JVL_ASSERT(args.size() == 1, "unary negation should have exactly one argument");
		return fmt::format("-{}", args[0]);
	}

	// Should be left with purely binary operations
	JVL_ASSERT(args.size() == 2,
		"$({}) is expected to take exactly two arguments",
		tbl_operation_code[code]);
	
	JVL_ASSERT(operators.contains(code),
		"no operator symbol found for $({})",
		tbl_operation_code[code]);

	const char *const op = operators.at(code);

	return fmt::format("({} {} {})", args[0], op, args[1]);
}

c_like_generator_t::c_like_generator_t(const body_t &body)
	: body_t(body), indentation(1) {}

void c_like_generator_t::finish(const std::string &s, bool semicolon)
{
	source += std::string(indentation << 2, ' ') + s + (semicolon ? ";" : "") + "\n";
}

void c_like_generator_t::declare(const std::string &t, index_t index)
{
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {}", t, var);
	local_variables[index] = var;
	finish(stmt);
}

void c_like_generator_t::define(const std::string &t, const std::string &v, index_t index)
{
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {} = {}", t, var, v);
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

	if (auto global = atom.get <Qualifier> ()) {
		return generate_global_reference(*global);
	} else if (auto load = atom.get <Load> ()) {
		std::string accessor;
		if (load->idx != -1)
			accessor = fmt::format(".f{}", load->idx);
		return reference(load->src) + accessor;
	} else if (auto swizzle = atom.get <Swizzle> ()) {
		std::string accessor = tbl_swizzle_code[swizzle->code];
		return reference(swizzle->src) + "." + accessor;
	}

	JVL_ABORT("failed to generate reference for: {} (@{})", atom, index);
}

std::string c_like_generator_t::inlined(index_t index) const
{
	JVL_ASSERT(index != -1, "invalid index passed to inlined");

	if (local_variables.count(index))
		return local_variables.at(index);

	const Atom &atom = atoms[index];

	if (auto prim = atom.get <Primitive> ()) {
		return generate_primitive(*prim);
	} else if (auto global = atom.get <Qualifier> ()) {
		return generate_global_reference(*global);
	} else if (auto ctor = atom.get <Construct> ()) {
		std::string t = generate_type_string(ctor->type, -1);
		return t + "(" + inlined(ctor->args) + ")";
	} else if (auto call = atom.get <Call> ()) {
		ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
		std::string args;
		if (call->args != -1)
			args = arguments_to_string(arguments(call->args));
		return fmt::format("{}{}", cbl->name, args);
	} else if (auto list = atom.get <List> ()) {
		std::string v = inlined(list->item);
		if (list->next != -1)
			v += ", " + inlined(list->next);
		return v;
	} else if (auto load = atom.get <Load> ()) {
		std::string accessor;
		if (load->idx != -1)
			accessor = fmt::format(".f{}", load->idx);
		return inlined(load->src) + accessor;
	} else if (auto swizzle = atom.get <Swizzle> ()) {
		std::string accessor = tbl_swizzle_code[swizzle->code];
		return inlined(swizzle->src) + "." + accessor;
	} else if (auto op = atom.get <Operation> ()) {
		return generate_operation(op->code, arguments(op->args));
	} else if (auto intr = atom.get <Intrinsic> ()) {
		auto args = arguments(intr->args);
		return tbl_intrinsic_operation[intr->opn] + arguments_to_string(args);
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

std::string c_like_generator_t::generate_type_string(index_t index, index_t field) const
{
	auto &atom = atoms[index];
	if (struct_names.contains(index)) {
		if (field == -1)
			return struct_names.at(index);

		JVL_ASSERT_PLAIN(atom.is <TypeInformation> ());
		if (field > 0) {
			index_t i = atom.as <TypeInformation> ().next;
			// TODO: inlined loop
			return generate_type_string(i, field - 1);
		}
	}

	if (auto global = atom.get <Qualifier> ()) {
		return generate_type_string(global->underlying, field);
	} else if (auto ctor = atom.get <Construct> ()) {
		return generate_type_string(ctor->type, field);
	} else if (auto tf = atom.get <TypeInformation> ()) {
		if (tf->down != -1)
			return generate_type_string(tf->down, -1);
		if (tf->item != bad)
			return tbl_primitive_types[tf->item];
	}

	JVL_ABORT("failed to resolve type name for {}", atom);
}

// Generators for each kind of instruction
template <>
void c_like_generator_t::generate <Qualifier> (const Qualifier &, index_t)
{
	// Same idea here; all globals should be
	// instatiated from the linker side
}

template <>
void c_like_generator_t::generate <TypeInformation> (const TypeInformation &, index_t)
{
	// Nothing to do here, linkage should have taken
	// care of generating the necessary structs; their
	// assigned names should all be in struct_names
}

template <>
void c_like_generator_t::generate <Primitive> (const Primitive &primitive, index_t index)
{
	std::string t = tbl_primitive_types[primitive.type];
	std::string v = generate_primitive(primitive);
	define(t, v, index);
}

template <>
void c_like_generator_t::generate <Swizzle> (const Swizzle &swizzle, index_t index)
{
	std::string t = "float";
	std::string v = inlined(index);
	define(t, v, index);
}

template <>
void c_like_generator_t::generate <Operation> (const Operation &operation, index_t index)
{
	std::string t = generate_type_string(operation.type, -1);
	std::string v = generate_operation(operation.code, arguments(operation.args));
	define(t, v, index);
}

template <>
void c_like_generator_t::generate <Intrinsic> (const Intrinsic &intrinsic, index_t index)
{
	// Keyword intrinsic
	if (intrinsic.type == -1)
		return finish(tbl_intrinsic_operation[intrinsic.opn]);

	auto &atom = atoms[intrinsic.type];
	if (!atom.is <TypeInformation> ())
		return finish("?", false);

	TypeInformation tf = atom.as <TypeInformation> ();
	if (tf.item == none) {
		// Void return type, so no assignment
		auto args = arguments(intrinsic.args);
		std::string v = tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
		return finish(v);
	} else {
		auto args = arguments(intrinsic.args);
		std::string t = generate_type_string(intrinsic.type, -1);
		std::string v = tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
		return define(t, v, index);
	}
}

template <>
void c_like_generator_t::generate <Construct> (const Construct &construct, index_t index)
{
	std::string t = generate_type_string(construct.type, -1);
	if (construct.args == -1) {
		declare(t, index);
	} else {
		std::string args = arguments_to_string(arguments(construct.args));
		std::string v = t + args;
		define(t, v, index);
	}
}

template <>
void c_like_generator_t::generate <Call> (const Call &call, index_t index)
{
	ire::Callable *cbl = ire::Callable::search_tracked(call.cid);
	std::string args = "()";
	if (call.args != -1)
		args = arguments_to_string(arguments(call.args));

	std::string t = generate_type_string(call.type, -1);
	std::string v = cbl->name + args;
	define(t, v, index);
}

template <>
void c_like_generator_t::generate <Store> (const Store &store, index_t)
{
	assign(store.dst, inlined(store.src));
}

template <>
void c_like_generator_t::generate <Load> (const Load &load, index_t index)
{
	std::string accessor;
	if (load.idx != -1)
		accessor = fmt::format(".f{}", load.idx);

	std::string t = generate_type_string(load.src, load.idx);
	std::string v = inlined(load.src) + accessor;
	define(t, v, index);
}

template <>
void c_like_generator_t::generate <Branch> (const Branch &branch, index_t)
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

	default:
		break;
	}
	
	JVL_ABORT("failed to generate branch: {}", branch);	
}

template <>
void c_like_generator_t::generate <Returns> (const Returns &returns, index_t)
{
	finish("return " + inlined(returns.value));
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
	for (int i = 0; i < atoms.size(); i++) {
		if (synthesized.count(i))
			generate(i);
	}

	return source;
}

std::string generate_c_like(const body_t &body)
{
	c_like_generator_t generator(body);
	return generator.generate();
}

} // namespace jvl::thunder::detail
