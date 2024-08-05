#include <cassert>
#include <map>

#include "ire/emitter.hpp"

namespace jvl::ire {

Emitter::Emitter() : pool(nullptr), dual(nullptr), size(0), pointer(0) {}

void Emitter::compact()
{
	pointer = detail::ir_compact_deduplicate(pool, dual, main, pointer);
	std::memset(pool, 0, size * sizeof(op::General));
	std::memcpy(pool, dual, pointer * sizeof(op::General));
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
	main.insert(p);
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
	} else {
		printf("op not conditional, is actually:");
		std::visit(op::dump_vd(), op);
		printf("\n");
		assert(false);
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
	wrapped::hash_table<int, std::string> struct_names;

	int struct_index = 0;
	for (int i = 0; i < pointer; i++) {
		op::General g = pool[i];
		if (!g.is<op::TypeField>())
			continue;

		// TODO: skip if unused as well

		op::TypeField tf = g.as<op::TypeField>();
		// TODO: skip if its only one primitive
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

	for (int i = 0; i < pointer; i++) {
		auto op = pool[i];
		if (!std::holds_alternative<op::Global>(op))
			continue;

		auto global = std::get<op::Global>(op);
		auto type = std::visit(op::typeof_vd(pool, struct_names), op);
		if (global.qualifier == op::Global::layout_in)
			lins[global.binding] = type;
		else if (global.qualifier == op::Global::layout_out)
			louts[global.binding] = type;
	}

	// Actual translation
	op::translate_glsl_vd tdisp;
	tdisp.pool = pool;
	tdisp.struct_names = struct_names;

	// Global shader variables
	// TODO: check for vulkan target, etc
	for (const auto &[binding, type] : lins)
		source += fmt::format("layout (location = {}) in {} _lin{};\n", binding,
				      type, binding);
	source += "\n";

	// TODO: remove extra space
	for (const auto &[binding, type] : louts)
		source += fmt::format("layout (location = {}) out {} _lout{};\n", binding,
				      type, binding);
	source += "\n";

	// Main function
	source += "void main()\n";
	source += "{\n";
	for (int i : main)
		source += "    " + tdisp.eval(i);
	source += "}\n";

	return source;
}

// Printing the IR state
// TODO: debug only?
void Emitter::dump()
{
	printf("GLOBALS (%4d/%4d)\n", pointer, size);
	for (size_t i = 0; i < pointer; i++) {
		if (std::ranges::find(main.begin(), main.end(), i) != main.end())
			printf("[*] ");
		else
			printf("    ");

		printf("[%4d]: ", i);
		std::visit(op::dump_vd(), pool[i]);
		printf("\n");
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
