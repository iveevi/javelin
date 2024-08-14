#include <cassert>
#include <fmt/core.h>
#include <map>

#include "atom/atom.hpp"
#include "ire/emitter.hpp"
#include "ire/tagged.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

Emitter::Emitter() : pointer(0) {}

void Emitter::clear()
{
	pointer = 0;
	std::memset(pool.data(), 0, pool.size() * sizeof(atom::General));

	// Reset active cache members
	for (auto &ref : caches)
		ref.get() = cache_index_t::null();

	// Reset usages
	used.clear();
	synthesized.clear();
}

void Emitter::compact()
{
	std::vector <atom::General> dual(pool.size());
	wrapped::reindex <atom::index_t> reindexer;
	std::tie(pointer, reindexer) = detail::ir_compact_deduplicate(pool.data(), dual.data(), synthesized, pointer);
	std::memset(pool.data(), 0, pool.size() * sizeof(atom::General));
	std::memcpy(pool.data(), dual.data(), pointer * sizeof(atom::General));
	synthesized = reindexer(synthesized);
	used = reindexer(used);
}

// State management
cache_index_t &Emitter::persist_cache_index(cache_index_t &index)
{
	caches.push_back(std::ref(index));
	return index;
}

// Dead code elimination
void Emitter::mark_used(int index, bool syn)
{
	if (index == -1)
		return;

	used.insert(index);

	atom::General g = pool[index];
	if (auto ctor = g.get <atom::Construct> ()) {
		mark_used(ctor->type, true);
		mark_used(ctor->args, true);
	} else if (auto list = g.get <atom::List> ()) {
		mark_used(list->item, false);
		mark_used(list->next, false);
		syn = false;
	} else if (auto load = g.get <atom::Load> ()) {
		mark_used(load->src, true);
	} else if (auto store = g.get <atom::Store> ()) {
		mark_used(store->src, false);
		mark_used(store->dst, false);
		syn = false;
	} else if (auto global = g.get <atom::Global> ()) {
		mark_used(global->type, true);
		syn = false;
	} else if (auto tf = g.get <atom::TypeField> ()) {
		mark_used(tf->down, false);
		mark_used(tf->next, false);
	} else if (auto op = g.get <atom::Operation> ()) {
		mark_used(op->args, false);
	} else if (auto intr = g.get <atom::Intrinsic> ()) {
		mark_used(intr->args, false);
	}

	if (syn)
		synthesized.insert(index);
}

void Emitter::mark_synthesized_underlying(int index)
{
	atom::General g = pool[index];
	if (auto op = g.get <atom::Operation> ())
		mark_synthesized_underlying(op->args);
}

// Emitting instructions during function invocation
int Emitter::emit(const atom::General &op)
{
	if (pointer >= pool.size())
		pool.resize(1 + (pool.size() << 2));

	pool[pointer] = op;

	return pointer++;
}

int Emitter::emit_main(const atom::General &op)
{
	int p = emit(op);
	synthesized.insert(p);
	mark_used(p, true);
	return p;
}

int Emitter::emit_main(const atom::Cond &cond)
{
	int p = emit_main((atom::General) cond);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::Elif &elif)
{
	int p = emit_main((atom::General) elif);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::While &loop)
{
	int p = emit_main((atom::General) loop);
	control_flow_ends.push(p);
	return p;
}

int Emitter::emit_main(const atom::End &end)
{
	int p = emit_main((atom::General) end);
	assert(control_flow_ends.size());
	auto ref = control_flow_ends.top();
	control_flow_ends.pop();
	control_flow_callback(ref, p);
	return p;
}

void Emitter::control_flow_callback(int ref, int p)
{
	auto &op = pool[ref];
	if (op.is <atom::Cond> ()) {
		op.as <atom::Cond> ().failto = p;
	} else if (op.is <atom::Elif> ()) {
		op.as <atom::Elif> ().failto = p;
	} else if (op.is <atom::While> ()) {
		op.as <atom::While> ().failto = p;
	} else {
		fmt::print("op not conditional, is actually:");
			atom::dump_ir_operation(op);
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

		atom::General g = pool[i];
		if (!g.is <atom::Global>())
			continue;

		atom::Global global = g.as <atom::Global> ();
		if (global.qualifier == atom::Global::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == atom::Global::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

// Kernel transfer
atom::Kernel Emitter::export_to_kernel()
{
	// Compaction, validation, ...
	compact();
	validate();

	atom::Kernel kernel;
	kernel.atoms.resize(pointer);
	std::memcpy(kernel.atoms.data(), pool.data(), sizeof(atom::General) * pointer);
	kernel.synthesized = synthesized;
	kernel.used = used;

	// No longer "owns" the IR
	clear();

	return kernel;
}

// Printing the IR state
// TODO: debug only?
void Emitter::dump()
{
	fmt::println("------------------------------");
	fmt::println("GLOBALS ({}/{})", pointer, pool.size());
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
			atom::dump_ir_operation(pool[i]);
		fmt::print("\n");
	}
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire
