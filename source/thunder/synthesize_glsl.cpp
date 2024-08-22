#include <functional>
#include <string>

#include "thunder/atom.hpp"
#include "ire/callable.hpp"
#include "thunder/enumerations.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

std::string glsl_global_ref(const Global &global)
{
	switch (global.qualifier) {
	case GlobalQualifier::layout_in:
		return fmt::format("_lin{}", global.binding);
	case GlobalQualifier::layout_out:
		return fmt::format("_lout{}", global.binding);
	case GlobalQualifier::parameter:
		return fmt::format("_arg{}", global.binding);
	case GlobalQualifier::push_constant:
		return "_pc";
	case GlobalQualifier::glsl_vertex_intrinsic_gl_Position:
		return "gl_Position";
	default:
		break;
	}

	return "<glo:?>";
}

std::string glsl_primitive(const Primitive &p)
{
	switch (p.type) {
	case i32:
		return fmt::format("{}", p.idata);
	case f32:
		return fmt::format("{}", p.fdata);
	default:
		break;
	}

	return "<prim:?>";
}

std::string glsl_format_operation(int type, const std::vector <std::string> &args)
{
	// TODO: check # of args

	switch (type) {
	case OperationCode::unary_negation:
		return fmt::format("-{}", args[0]);
	case OperationCode::addition:
		return fmt::format("({} + {})", args[0], args[1]);
	case OperationCode::subtraction:
		return fmt::format("({} - {})", args[0], args[1]);
	case OperationCode::multiplication:
		return fmt::format("({} * {})", args[0], args[1]);
	case OperationCode::division:
		return fmt::format("({} / {})", args[0], args[1]);
	case OperationCode::cmp_ge:
		return fmt::format("({} > {})", args[0], args[1]);
	case OperationCode::cmp_le:
		return fmt::format("({} < {})", args[0], args[1]);
	case OperationCode::cmp_geq:
		return fmt::format("({} >= {})", args[0], args[1]);
	case OperationCode::cmp_leq:
		return fmt::format("({} <= {})", args[0], args[1]);
	default:
		break;
	}

	return "<op:?>";
}

std::string synthesize_glsl_body(const Atom *const pool,
		                 const wrapped::hash_table <int, std::string> &struct_names,
		                 const std::unordered_set <index_t> &synthesized,
				 size_t size)
{
	wrapped::hash_table <int, std::string> variables;

	std::function <std::string (int)> inlined;
	std::function <std::string (int)> ref;

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

	ref = [&](int index) -> std::string {
		if (index == -1) {
			fmt::println("invalid index passed to ref");
			abort();
		}

		Atom g = pool[index];
		if (auto global = g.get <Global> ()) {
			return glsl_global_ref(*global);
		} else if (auto load = g.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);
			return ref(load->src) + accessor;
		} else if (auto swizzle = g.get <Swizzle> ()) {
			std::string accessor = tbl_swizzle_code[swizzle->code];
			return ref(swizzle->src) + "." + accessor;
		} else if (!variables.count(index)) {
			fmt::println("unexpected IR requested for ref(...)");
			dump_ir_operation(g);
			fmt::print("\n");
			abort();
		}

		return variables[index];
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
			Atom h = pool[l];
			if (!h.is <List> ()) {
				fmt::println("unexpected atom in arglist:");
				dump_ir_operation(h);
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

	inlined = [&](int index) -> std::string {
		if (variables.count(index))
			return variables[index];

		if (index == -1) {
			fmt::println("invalid index passed to inlined");
			abort();
		}

		Atom g = pool[index];

		if (auto prim = g.get <Primitive> ()) {
			return glsl_primitive(*prim);
		} else if (auto global = g.get <Global> ()) {
			return glsl_global_ref(*global);
		} else if (auto ctor = g.get <Construct> ()) {
			std::string t = type_name(pool, struct_names, ctor->type, -1);
			return t + "(" + inlined(ctor->args) + ")";
		} else if (auto call = g.get <Call> ()) {
			ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
			std::string args;
			if (call->args != -1)
				args = strargs(arglist(call->args));
			return fmt::format("{}{}", cbl->name, args);
		} else if (auto list = g.get <List> ()) {
			std::string v = inlined(list->item);
			if (list->next != -1)
				v += ", " + inlined(list->next);

			return v;
		} else if (auto load = g.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);
			return inlined(load->src) + accessor;
		} else if (auto swizzle = g.get <Swizzle> ()) {
			std::string accessor = tbl_swizzle_code[swizzle->code];
			return ref(swizzle->src) + "." + accessor;
		} else if (auto op = g.get <Operation> ()) {
			return glsl_format_operation(op->code, arglist(op->args));
		} else if (auto intr = g.get <Intrinsic> ()) {
			auto args = arglist(intr->args);
			return intr->name + strargs(args);
		} else {
			fmt::println("not sure how to inline atom:");
			dump_ir_operation(g);
			fmt::print("\n");
			// abort();
		}

		return "<inl:?>";
	};

	auto intrinsic = [&](const Intrinsic &intr, int index) -> std::string {
		// Keyword intrinsic
		if (intr.type == -1)
			return finish(intr.name);

		Atom g = pool[intr.type];
		if (!g.is <TypeField> ())
			return "?";

		TypeField tf = g.as <TypeField> ();
		if (tf.item == none) {
			// Void return type, so no assignment
			auto args = arglist(intr.args);
			std::string v = intr.name + strargs(args);
			return finish(v);
		} else {
			auto args = arglist(intr.args);
			std::string t = type_name(pool, struct_names, intr.type, -1);
			std::string v = intr.name + strargs(args);
			return assign_new(t, v, index);
		}
	};

	// Final emitted source code
	std::string source;

	auto synthesize = [&](const Atom &g, int index) -> void {
		// TODO: index-based switch?
		if (auto branch = g.get <Branch> ()) {
			std::string v = inlined(branch->cond);
			std::string ifs = "if (" + v + ") {";
			source += finish(ifs, false);
			indentation++;
		} else if (auto loop = g.get <While> ()) {
			std::string v = inlined(loop->cond);
			std::string whiles = "while (" + v + ") {";
			source += finish(whiles, false);
			indentation++;
		// } else if (auto elif = g.get <Elif> ()) {
		// 	indentation--;

		// 	std::string elifs = "} else {";
		// 	if (elif->cond != -1) {
		// 		std::string v = inlined(elif->cond);
		// 		std::string ifs = "} else if (" + v + ") {";
		// 	}

		// 	source += finish(elifs, false);

		// 	indentation++;
		} else if (auto ctor = g.get <Construct> ()) {
			std::string t = type_name(pool, struct_names, ctor->type, -1);
			if (ctor->args == -1) {
				source += declare_new(t, index);
			} else {
				std::string args = strargs(arglist(ctor->args));
				std::string v = t + args;
				source += assign_new(t, v, index);
			}
		} else if (auto call = g.get <Call> ()) {
			ire::Callable *cbl = ire::Callable::search_tracked(call->cid);
			std::string args = "()";
			if (call->args != -1)
				args = strargs(arglist(call->args));

			std::string t = type_name(pool, struct_names, call->type, -1);
			std::string v = cbl->name + args;
			source += assign_new(t, v, index);
		} else if (auto load = g.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);

			std::string t = type_name(pool, struct_names, load->src, load->idx);
			std::string v = inlined(load->src) + accessor;
			source += assign_new(t, v, index);
		} else if (auto swizzle = g.get <Swizzle> ()) {
			// TODO: generalize
			std::string t = "float";
			std::string v = inlined(index);
			source += assign_new(t, v, index);
		} else if (auto store = g.get <Store> ()) {
			// TODO: there could be a store index chain...
			std::string v = inlined(store->src);
			source += assign_to(store->dst, v);
		} else if (auto prim = g.get <Primitive> ()) {
			std::string t = tbl_primitive_types[prim->type];
			std::string v = glsl_primitive(*prim);
			source += assign_new(t, v, index);
		} else if (auto intr = g.get <Intrinsic> ()) {
			source += intrinsic(intr.value(), index);
		} else if (auto op = g.get <Operation> ()) {
			// TODO: lambda shortcut
			std::string t = type_name(pool, struct_names, op->type, -1);
			std::string v = glsl_format_operation(op->code, arglist(op->args));
			source += assign_new(t, v, index);
		} else if (auto ret = g.get <Returns> ()) {
			// TODO: create a tuple type struct if necesary
			auto args = arglist(ret->args);
			source += finish("return " + strargs(args));
		} else if (g.get <End> ()) {
			indentation--;
			source += finish("}", false);
		} else if (g.is <TypeField> ()) {
			// Already taken care of during type/struct synthesis
		} else if (g.is <Global> ()) {
			// Same reason; otherwise there are shader intrinsics
			// which do not need to be synthesized
		} else {
			// TODO: error reporting for jvl facilities
			fmt::println("unexpected IR requested for synthesize(...)");
			dump_ir_operation(g);
			fmt::print("\n");

			source += finish("<?>", false);
		}
	};

	for (int i = 0; i < size; i++) {
		if (!synthesized.count(i))
			continue;

		synthesize(pool[i], i);
	}

	return source;
}

} // namespace jvl::ire::atom
