#include <functional>
#include <string>

#include "ire/callable.hpp"
#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder::detail {

MODULE(generate-body-c-like);

// TODO: method for body_t
std::string type_name(const std::vector <Atom> &pool,
		      const wrapped::hash_table <int, std::string> &struct_names,
		      index_t index,
		      index_t field)
{
	Atom g = pool[index];
	if (struct_names.contains(index)) {
		if (field == -1)
			return struct_names.at(index);

		JVL_ASSERT_PLAIN(g.is <TypeInformation> ());
		if (field > 0) {
			index_t i = g.as <TypeInformation> ().next;
			// TODO: traverse inline
			return type_name(pool, struct_names, i, field - 1);
		}
	}

	if (auto global = g.get <Qualifier> ()) {
		return type_name(pool, struct_names, global->underlying, field);
	} else if (auto ctor = g.get <Construct> ()) {
		return type_name(pool, struct_names, ctor->type, field);
	} else if (auto tf = g.get <TypeInformation> ()) {
		if (tf->down != -1)
			return type_name(pool, struct_names, tf->down, -1);
		if (tf->item != bad)
			return tbl_primitive_types[tf->item];
	} else {
		fmt::println("failed to resolve type name for: {}", g.to_string());
	}

	return "<BAD>";
}

// TODO: pass options to the body_t type
std::string generate_global_reference(const Qualifier &global)
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

// TODO: take body_t as argument with all of these
std::string generate_c_like(const body_t &body)
{
	auto &atoms = body.atoms;
	auto &struct_names = body.struct_names;
	auto &synthesized = body.synthesized;

	wrapped::hash_table <index_t, std::string> variables;

	std::function <std::string (index_t)> inlined;
	std::function <std::string (index_t)> ref;

	int indentation = 1;
	auto finish = [&](const std::string &s, bool semicolon = true) {
		return std::string(indentation << 2, ' ') + s + (semicolon ? ";" : "") + "\n";
	};

	auto declare_new = [&](const std::string &t, int index) -> std::string {
		int n = variables.size();
		std::string var = fmt::format("s{}", n);
		std::string stmt = fmt::format("{} {}", t, var);
		variables[index] = var;
		return finish(stmt);
	};

	auto assign_new = [&](const std::string &t, const std::string &v, int index) -> std::string {
		int n = variables.size();
		std::string var = fmt::format("s{}", n);
		std::string stmt = fmt::format("{} {} = {}", t, var, v);
		variables[index] = var;
		return finish(stmt);
	};

	ref = [&](index_t index) -> std::string {
		JVL_ASSERT(index != -1, "invalid index passed to ref");

		if (variables.count(index))
			return variables[index];

		const Atom &atom = atoms[index];

		if (auto global = atom.get <Qualifier> ()) {
			return generate_global_reference(*global);
		} else if (auto load = atom.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);
			return ref(load->src) + accessor;
		} else if (auto swizzle = atom.get <Swizzle> ()) {
			std::string accessor = tbl_swizzle_code[swizzle->code];
			return ref(swizzle->src) + "." + accessor;
		}

		JVL_ABORT("cannot generate referenced alias to: {} (@{})", atom, index);
	};

	auto assign_to = [&](int index, const std::string &v) -> std::string {
		std::string stmt = fmt::format("{} = {}", ref(index), v);
		return finish(stmt);
	};

	auto strargs = [](const std::vector <std::string> &args) -> std::string {
		std::string ret;
		ret += "(";
		for (size_t i = 0; i < args.size(); i++) {
			ret += args[i];
			if (i + 1 < args.size())
				ret += ", ";
		}

		ret += ")";
		return ret;
	};

	auto arglist = [&](int start) -> std::vector <std::string> {
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
	};

	inlined = [&](index_t index) -> std::string {
		JVL_ASSERT(index != -1, "invalid index passed to inlined");

		if (variables.count(index))
			return variables[index];

		const Atom &atom = atoms[index];

		if (auto prim = atom.get <Primitive> ()) {
			return generate_primitive(*prim);
		} else if (auto global = atom.get <Qualifier> ()) {
			return generate_global_reference(*global);
		} else if (auto ctor = atom.get <Construct> ()) {
			std::string t = type_name(atoms, struct_names, ctor->type, -1);
			return t + "(" + inlined(ctor->args) + ")";
		} else if (auto call = atom.get <Call> ()) {
			ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
			std::string args;
			if (call->args != -1)
				args = strargs(arglist(call->args));
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
			return generate_operation(op->code, arglist(op->args));
		} else if (auto intr = atom.get <Intrinsic> ()) {
			auto args = arglist(intr->args);
			return tbl_intrinsic_operation[intr->opn] + strargs(args);
		}

		JVL_ABORT("failed to inline atom: {} (@{})", atom, index);
	};

	auto intrinsic = [&](const Intrinsic &intr, int index) -> std::string {
		// Keyword intrinsic
		if (intr.type == -1)
			return finish(tbl_intrinsic_operation[intr.opn]);

		Atom g = atoms[intr.type];
		if (!g.is <TypeInformation> ())
			return "?";

		TypeInformation tf = g.as <TypeInformation> ();
		if (tf.item == none) {
			// Void return type, so no assignment
			auto args = arglist(intr.args);
			std::string v = tbl_intrinsic_operation[intr.opn] + strargs(args);
			return finish(v);
		} else {
			auto args = arglist(intr.args);
			std::string t = type_name(atoms, struct_names, intr.type, -1);
			std::string v = tbl_intrinsic_operation[intr.opn] + strargs(args);
			return assign_new(t, v, index);
		}
	};

	// Final emitted source code
	std::string source;

	auto synthesize = [&](const Atom &atom, int index) -> void {
		// TODO: index-based switch?
		if (auto branch = atom.get <Branch> ()) {
			switch (branch->kind) {

			case control_flow_end:
				indentation--;
				source += finish("}", false);
				break;

			case conditional_if:
				source += finish(fmt::format("if ({}) {{", inlined(branch->cond)), false);
				indentation++;
				break;
			
			case loop_while:
				source += finish(fmt::format("while ({}) {{", inlined(branch->cond)), false);
				indentation++;
				break;
			
			case conditional_else_if:
				indentation--;
				source += finish(fmt::format("}} else if ({}) {{", inlined(branch->cond)), false);
				indentation++;
				break;
			
			case conditional_else:
				indentation--;
				source += finish("} else {", false);
				indentation++;
				break;

			default:
				JVL_ABORT("unhandled branch case in synthesize: {}", atom);	
			}
		} else if (auto ctor = atom.get <Construct> ()) {
			std::string t = type_name(atoms, struct_names, ctor->type, -1);
			if (ctor->args == -1) {
				source += declare_new(t, index);
			} else {
				std::string args = strargs(arglist(ctor->args));
				std::string v = t + args;
				source += assign_new(t, v, index);
			}
		} else if (auto call = atom.get <Call> ()) {
			ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
			std::string args = "()";
			if (call->args != -1)
				args = strargs(arglist(call->args));

			std::string t = type_name(atoms, struct_names, call->type, -1);
			std::string v = cbl->name + args;
			source += assign_new(t, v, index);
		} else if (auto load = atom.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);

			std::string t = type_name(atoms, struct_names, load->src, load->idx);
			std::string v = inlined(load->src) + accessor;
			source += assign_new(t, v, index);
		} else if (auto swizzle = atom.get <Swizzle> ()) {
			// TODO: generalize
			std::string t = "float";
			std::string v = inlined(index);
			source += assign_new(t, v, index);
		} else if (auto store = atom.get <Store> ()) {
			// TODO: there could be a store index chain...
			std::string v = inlined(store->src);
			source += assign_to(store->dst, v);
		} else if (auto prim = atom.get <Primitive> ()) {
			std::string t = tbl_primitive_types[prim->type];
			std::string v = generate_primitive(*prim);
			source += assign_new(t, v, index);
		} else if (auto intr = atom.get <Intrinsic> ()) {
			source += intrinsic(intr.value(), index);
		} else if (auto op = atom.get <Operation> ()) {
			// TODO: lambda shortcut
			std::string t = type_name(atoms, struct_names, op->type, -1);
			std::string v = generate_operation(op->code, arglist(op->args));
			source += assign_new(t, v, index);
		} else if (auto ret = atom.get <Returns> ()) {
			// TODO: create a tuple type struct if necesary
			source += finish("return " + inlined(ret->value));
		} else if (atom.is <TypeInformation> ()) {
			// Already taken care of during type/struct synthesis
		} else if (atom.is <Qualifier> ()) {
			// Same reason; otherwise there are shader intrinsics
			// which do not need to be synthesized
		} else {
			JVL_ABORT("unexpected IR requested for synthesize(...): {}", atom.to_string());
		}
	};

	for (int i = 0; i < atoms.size(); i++) {
		if (synthesized.count(i))
			synthesize(atoms[i], i);
	}

	return source;
}

} // namespace jvl::thunder::detail
