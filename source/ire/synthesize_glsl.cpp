#include <functional>
#include <string>

#include "ire/op.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire::op {

std::string glsl_global_ref(const Global &global)
{
	switch (global.qualifier) {
	case Global::layout_out:
		return "_lout" + std::to_string(global.binding);
	case Global::layout_in:
		return "_lin" + std::to_string(global.binding);
	case Global::push_constant:
		return "_pc";
	case Global::glsl_vertex_intrinsic_gl_Position:
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
		return fmt::format("{}", p.idata[0]);
	case f32:
		return fmt::format("{}", p.fdata[0]);
	case vec4:
		return fmt::format("vec4({}, {}, {}, {})",
				p.fdata[0],
				p.fdata[1],
				p.fdata[2],
				p.fdata[3]);
	default:
		break;
	}

	return "<prim:?>";
}

std::string glsl_cmp(const Cmp &cmp)
{
	switch (cmp.type) {
	case Cmp::eq:
		return "==";
	default:
		break;
	}

	return "<cmp:?>";
}

std::string synthesize_glsl_body(const General *const pool,
		                 const wrapped::hash_table <int, std::string> &struct_names,
		                 const std::unordered_set <int> &synthesized,
				 size_t size)
{
	wrapped::hash_table <int, std::string> variables;

	int indentation = 1;
	auto finish = [&](const std::string &s, bool semicolon = true) {
		return std::string(indentation << 2, ' ') + s + (semicolon ? ";" : "") + "\n";
	};

	auto assign_new = [&](const std::string &t, const std::string &v, int index) -> std::string {
		int n = variables.size();
		std::string var = fmt::format("s{}", n);
		std::string stmt = fmt::format("{} {} = {}", t, var, v);
		variables[index] = var;
		return finish(stmt);
	};

	std::function <std::string (int)> ref;
	ref = [&](int index) -> std::string {
		General g = pool[index];
		if (auto global = g.get <Global> ()) {
			return glsl_global_ref(*global);
		} else if (auto load = g.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);
			return ref(load->src) + accessor;
		} else if (auto swizzle = g.get <Swizzle> ()) {
			std::string accessor = Swizzle::swizzle_name[swizzle->type];
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

	std::function <std::string (int)> inlined;
	inlined = [&](int index) -> std::string {
		if (variables.count(index))
			return variables[index];

		General g = pool[index];

		if (auto prim = g.get <Primitive> ()) {
			return glsl_primitive(*prim);
		} else if (auto global = g.get <Global> ()) {
			return glsl_global_ref(*global);
		} else if (auto ctor = g.get <Construct> ()) {
			std::string t = type_name(pool, struct_names, ctor->type, -1);
			return t + "(" + inlined(ctor->args) + ")";
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
			std::string accessor = Swizzle::swizzle_name[swizzle->type];
			return ref(swizzle->src) + "." + accessor;
		} else if (auto cmp = g.get <Cmp> ()) {
			return fmt::format("({} {} {})",
					inlined(cmp->a),
					glsl_cmp(*cmp),
					inlined(cmp->b));
		}

		return "?";
	};

	std::string source;
	auto synthesize = [&](const General &g, int index) -> void {
		if (auto cond = g.get <Cond> ()) {
			std::string v = inlined(cond->cond);
			std::string ifs = "if (" + v + ") {";
			source += finish(ifs, false);
			indentation++;
		} else if (auto loop = g.get <While> ()) {
			std::string v = inlined(loop->cond);
			std::string whiles = "while (" + v + ") {";
			source += finish(whiles, false);
			indentation++;
		} else if (auto elif = g.get <Elif> ()) {
			indentation--;

			std::string elifs = "} else {";
			if (elif->cond != -1) {
				std::string v = inlined(elif->cond);
				std::string ifs = "} else if (" + v + ") {";
			}

			source += finish(elifs, false);

			indentation++;
		} else if (auto ctor = g.get <Construct> ()) {
			std::string t = type_name(pool, struct_names, ctor->type, -1);
			std::string v = t + "(" + inlined(ctor->args) + ")";
			source += assign_new(t, v, index);
		} else if (auto load = g.get <Load> ()) {
			std::string accessor;
			if (load->idx != -1)
				accessor = fmt::format(".f{}", load->idx);

			std::string t = type_name(pool, struct_names, load->src, load->idx);
			std::string v = inlined(load->src) + accessor;
			source += assign_new(t, v, index);
		} else if (auto store = g.get <Store> ()) {
			// TODO: there could be a store index chain...
			std::string v = inlined(store->src);
			source += assign_to(store->dst, v);
		} else if (auto prim = g.get <Primitive> ()) {
			std::string t = type_table[prim->type];
			std::string v = glsl_primitive(*prim);
			source += assign_new(t, v, index);
		} else if (auto cmp = g.get <Cmp> ()) {
			std::string t = type_table[boolean];
			std::string v = fmt::format("({} {} {})",
						inlined(cmp->a),
						glsl_cmp(*cmp),
						inlined(cmp->b));

			source += assign_new(t, v, index);
		} else if (g.get <End> ()) {
			indentation--;
			source += finish("}", false);
		} else if (g.is <TypeField> ()) {
			// Already taken care of during type/struct synthesis
		} else if (g.is <Global> ()) {
			// Same reason; otherwise there are shader intrinsics
			// which do not need to be synthesized
		} else {
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

} // namespace jvl::ire::op
