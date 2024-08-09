#include <cassert>
#include <fmt/core.h>
#include <map>

#include "ire/emitter.hpp"
#include "ire/op.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

Emitter::Emitter() : pool(nullptr), dual(nullptr), size(0), pointer(0) {}

void Emitter::clear()
{
	pointer = 0;
	std::memset(pool, 0, size * sizeof(op::General));
	std::memset(dual, 0, size * sizeof(op::General));
}

void Emitter::compact()
{
	wrapped::reindex reindexer;
	std::tie(pointer, reindexer) = detail::ir_compact_deduplicate(pool, dual, synthesized, pointer);
	std::memset(pool, 0, size * sizeof(op::General));
	std::memcpy(pool, dual, pointer * sizeof(op::General));
	synthesized = reindexer(synthesized);
	used = reindexer(used);
}

void Emitter::resize(size_t units)
{
	size_t count = 0;

	// TODO: compact IR before resizing
	op::General *old_pool = pool;
	op::General *old_dual = dual;

	pool = new op::General[units];
	dual = new op::General[units];

	std::memset(pool, 0, units * sizeof(op::General));
	std::memset(dual, 0, units * sizeof(op::General));

	if (old_pool) {
		std::memcpy(pool, old_pool, size * sizeof(op::General));
		delete[] old_pool;
	}

	if (old_dual) {
		std::memcpy(dual, old_dual, size * sizeof(op::General));
		delete[] old_dual;
	}

	size = units;
}

// Dead code elimination
void Emitter::mark_used(int index, bool syn)
{
	if (index == -1)
		return;

	used.insert(index);

	op::General g = pool[index];
	if (auto ctor = g.get <op::Construct> ()) {
		mark_used(ctor->type, true);
		mark_used(ctor->args, true);
	} else if (auto list = g.get <op::List> ()) {
		mark_used(list->item, false);
		mark_used(list->next, false);
		syn = false;
	} else if (auto load = g.get <op::Load> ()) {
		mark_used(load->src, true);
	} else if (auto store = g.get <op::Store> ()) {
		mark_used(store->src, false);
		mark_used(store->dst, false);
		syn = false;
	} else if (auto global = g.get <op::Global> ()) {
		mark_used(global->type, true);
		syn = false;
	} else if (auto tf = g.get <op::TypeField> ()) {
		mark_used(tf->down, false);
		mark_used(tf->next, false);
	}

	if (syn)
		synthesized.insert(index);
}

void Emitter::mark_synthesized_underlying(int index)
{
	op::General g = pool[index];
	if (auto cmp = g.get <op::Cmp> ()) {
		mark_used(cmp->a, true);
		mark_used(cmp->b, true);
	}
}

// Emitting instructions during function invocation
int Emitter::emit(const op::General &op)
{
	if (pointer >= size) {
		if (size == 0)
			resize(1 << 4);
		else
			resize(size << 2);
	}

	if (pointer >= size) {
		printf("error, exceed global pool size, please reserve "
		       "beforehand\n");
		throw -1;
		return -1;
	}

	pool[pointer] = op;
	return pointer++;
}

int Emitter::emit_main(const op::General &op)
{
	int p = emit(op);
	synthesized.insert(p);
	mark_used(p, true);
	return p;
}

int Emitter::emit_main(const op::Cond &cond)
{
	int p = emit_main((op::General) cond);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const op::Elif &elif)
{
	int p = emit_main((op::General) elif);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const op::While &loop)
{
	int p = emit_main((op::General) loop);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const op::End &end)
{
	int p = emit_main((op::General) end);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	return p;
}

void Emitter::control_flow_callback(int ref, int p)
{
	auto &op = pool[ref];
	if (op.is <op::Cond> ()) {
		op.as <op::Cond> ().failto = p;
	} else if (op.is <op::Elif> ()) {
		op.as <op::Elif> ().failto = p;
	} else if (op.is <op::While> ()) {
		op.as <op::While> ().failto = p;
	} else {
		fmt::print("op not conditional, is actually:");
			op::dump_ir_operation(op);
		fmt::print("\n");

		abort();
	}
}

// Validating IR
void Emitter::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp

	wrapped::hash_table <int, int> inputs;
	wrapped::hash_table <int, int> outputs;

	for (int i = 0; i < pointer; i++) {
		if (!used.contains(i))
			continue;

		op::General g = pool[i];
		if (!g.is <op::Global>())
			continue;

		op::Global global = g.as <op::Global> ();
		if (global.qualifier == op::Global::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == op::Global::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

// Generating GLSL source code
std::string Emitter::generate_glsl()
{
	// TODO: mark instructions as unused (unless flags are given)

	// Final generated source
	std::string source;

	// Version header
	source += "#version 450\n";
	source += "\n";

	// Gather all necessary structs
	wrapped::hash_table <int, std::string> struct_names;

	int struct_index = 0;
	for (int i = 0; i < pointer; i++) {
		if (!synthesized.contains(i))
			continue;

		op::General g = pool[i];
		if (!g.is <op::TypeField>())
			continue;

		op::TypeField tf = g.as <op::TypeField>();
		if (tf.next == -1 && tf.down == -1)
			continue;

		// TODO: handle nested structs (down)

		std::string struct_name = fmt::format("s{}_t", struct_index++);
		struct_names[i] = struct_name;

		std::string struct_source;
		struct_source = fmt::format("struct {} {{\n", struct_name);

		int field_index = 0;
		int j = i;
		while (j != -1) {
			op::General g = pool[j];
			if (!g.is<op::TypeField>())
				abort();

			op::TypeField tf = g.as<op::TypeField>();
			// TODO: nested
			// TODO: put this whole thing in a method

			struct_source += fmt::format(
				"    {} f{};\n", op::type_table[tf.item], field_index++);

			j = tf.next;
		}

		struct_source += "};\n\n";

		source += struct_source;
	}

	// Gather all global shader variables
	std::map <int, std::string> lins;
	std::map <int, std::string> louts;
	std::string push_constant;

	for (int i = 0; i < pointer; i++) {
		if (!used.count(i))
			continue;

		auto op = pool[i];
		if (!op.is <op::Global> ())
			continue;

		auto global = std::get <op::Global> (op);
		auto type = op::type_name(pool, struct_names, i, -1);
		if (global.qualifier == op::Global::layout_in)
			lins[global.binding] = type;
		else if (global.qualifier == op::Global::layout_out)
			louts[global.binding] = type;
		else if (global.qualifier == op::Global::push_constant)
			push_constant = type;
	}

	// Global shader variables
	for (const auto &[binding, type] : lins) {
		source += fmt::format("layout (location = {}) in {} _lin{};\n",
				binding, type, binding);
	}

	if (lins.size())
		source += "\n";

	for (const auto &[binding, type] : louts)
		source += fmt::format("layout (location = {}) out {} _lout{};\n",
				binding, type, binding);

	if (louts.size())
		source += "\n";

	// TODO: check for vulkan or opengl, etc
	if (push_constant.size()) {
		source += "layout (push_constant) uniform constants\n";
		source += "{\n";
		source += "     " + push_constant + " _pc;\n";
		source += "}\n";
		source += "\n";
	}

	// Main function
	source += "void main()\n";
	source += "{\n";
	source += op::synthesize_glsl_body(pool, struct_names, synthesized, pointer);
	source += "}\n";

	return source;
}

// Printing the IR state
// TODO: debug only?
void Emitter::dump()
{
	fmt::println("------------------------------");
	fmt::println("GLOBALS ({}/{})", pointer, size);
	fmt::println("------------------------------");
	for (size_t i = 0; i < pointer; i++) {
		if (synthesized.contains(i))
			fmt::print("S");
		else
			fmt::print(" ");

		if (used.contains(i))
			fmt::print("U");
		else
			fmt::print(" ");

		fmt::print(" [{:4d}]: ", i);
			op::dump_ir_operation(pool[i]);
		fmt::print("\n");
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
