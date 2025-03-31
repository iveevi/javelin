#include <queue>
#include <unordered_set>

#include "common/logging.hpp"

#include "thunder/optimization.hpp"

namespace jvl::thunder {

MODULE(optimization);

bool casting_intrinsic(const Intrinsic &intr)
{
	switch (intr.opn) {
	case cast_to_int:
	case cast_to_ivec2:
	case cast_to_ivec3:
	case cast_to_ivec4:
	case cast_to_uint:
	case cast_to_uvec2:
	case cast_to_uvec3:
	case cast_to_uvec4:
	case cast_to_float:
	case cast_to_vec2:
	case cast_to_vec3:
	case cast_to_vec4:
	case cast_to_uint64:
		return true;
	default:
		break;
	}

	return false;
}

///////////////////////////////
// Optimizer implementations //
///////////////////////////////

Optimizer Optimizer::stable;

// Instruction distillation
uint32_t Optimizer::distill_types_once(Buffer &buffer) const
{
	static_assert(sizeof(Atom) == sizeof(uint64_t));
	
	uint32_t counter = 0;

	// Each atom converted to a 64-bit integer
	HashMap existing;

	// TODO: move outside...
	auto unique = [&](Index i) -> Index {
		auto &atom = buffer.atoms[i];
		auto &hash = reinterpret_cast <uint64_t &> (atom);
		
		if (!atom.is <TypeInformation> ())
			return i;

		auto it = existing.find(hash);
		if (it != existing.end()) {
			if (i != it->second) {
				counter++;
				buffer.marked.erase(i);
			}

			fmt::println("distilling types: %{} ==> %{}", i, it->second);

			return it->second;
		}

		existing[hash] = i;

		return i;
	};

	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];

		auto addrs = atom.addresses();
		if (addrs.a0 != -1)
			addrs.a0 = unique(addrs.a0);
		if (addrs.a1 != -1)
			addrs.a1 = unique(addrs.a1);
	}

	return counter;

}

void Optimizer::distill_types(Buffer &buffer) const
{
	uint32_t counter = 0;

	bool changed;
	do {
		uint32_t distilled = distill_types_once(buffer);
		changed = false;
		changed |= (distilled != 0);
		changed |= strip(buffer);
		counter++;
	} while (changed);

	JVL_INFO("ran distill types pass {} times", counter);
}

void Optimizer::distill(Buffer &buffer) const {}

// Instruction disolving
void Optimizer::disolve_casting(Relocation &relocation, const Buffer &buffer, const Intrinsic &intr) const
{
	fmt::println("casting instruction: {}", intr.to_assembly_string());

	auto list = buffer.atoms[intr.args].as <List> ();
	auto qt = buffer.types[list.item];

	fmt::println("\ttype is {}", qt);
}

void Optimizer::disolve_constructor(Relocation &relocation, const Buffer &buffer, const Construct &ctor, Index i) const
{
	fmt::println("constructor instruction: {}", ctor.to_assembly_string());
	if (ctor.args == -1)
		return;
	
	auto list = buffer.atoms[ctor.args].as <List> ();
	if (list.next == -1) {
		auto type_this = buffer.types[i];
		auto type_arg = buffer.types[list.item];

		if (type_this == type_arg) {
			fmt::println("\tvacous construction!");
			relocation.emplace(i, list.item);
		}
	}

	// TODO: elision through element-wise forwarding
}

void Optimizer::disolve_once(Buffer &buffer) const
{
	Relocation relocation;

	// Collection phase
	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];

		switch (atom.index()) {
		
		variant_case(Atom, Construct):
		{
			auto &ctor = atom.as <Construct> ();
			disolve_constructor(relocation, buffer, ctor, i);
		} break;

		variant_case(Atom, Intrinsic):
		{
			auto &intr = atom.as <Intrinsic> ();
			if (casting_intrinsic(intr))
				disolve_casting(relocation, buffer, intr);
		} break;

		default:
			break;
		}
	}

	// Replacement phase
	relocation.apply(buffer);
}

void Optimizer::disolve(Buffer &buffer) const
{
	uint32_t counter = 0;

	bool changed;
	do {
		disolve_once(buffer);
		changed = strip(buffer);
		counter++;
	} while (changed);

	JVL_INFO("ran disolve pass {} times", counter);
}

// Dead code elimination
bool Optimizer::strip_buffer(Buffer &buffer, const std::vector <bool> &include) const
{
	Index pointer = 0;

	Relocation relocation;
	for (size_t i = 0; i < buffer.pointer; i++) {
		if (include[i])
			relocation[i] = pointer++;
	}

	Buffer doubled;
	for (size_t i = 0; i < buffer.pointer; i++) {
		if (relocation.contains(i)) {
			relocation.apply(buffer.atoms[i]);
			doubled.emit(buffer.atoms[i]);
		}
	}

	// Transfer decorations
	doubled.decorations.all = buffer.decorations.all;

	for (auto &[i, j] : buffer.decorations.used) {
		auto k = relocation[i];
		doubled.decorations.used[k] = j;
	}
	
	for (auto &i : buffer.decorations.phantom) {
		auto k = relocation[i];
		doubled.decorations.phantom.insert(k);
	}

	std::swap(buffer, doubled);

	// Change happened only if there is a difference in size
	return (buffer.pointer != doubled.pointer);
}

bool Optimizer::strip_once(Buffer &buffer) const
{
	UsageGraph graph = usage(buffer);

	// Reversed usage graph
	// TODO: users graph...
	UsageGraph reversed(graph.size());
	for (size_t i = 0; i < graph.size(); i++) {
		for (Index j : graph[i])
			reversed[j].insert(i);
	}

	// Configure checking queue and inclusion mask
	std::vector <bool> include(buffer.pointer, true);
	
	std::queue <Index> check_list;
	for (size_t i = 0; i < buffer.pointer; i++)
		check_list.push(i);

	// Keep checking as long as something got erased
	while (check_list.size()) {
		std::set <Index> erasure;

		while (check_list.size()) {
			Index i = check_list.front();
			check_list.pop();
			
			if (graph[i].empty() && !buffer.marked.contains(i)) {
				include[i] = false;
				erasure.insert(i);
			}
		}

		for (auto i : erasure) {
			for (auto j : reversed[i]) {
				graph[j].erase(i);
				check_list.push(j);
			}
		}
	}

	// Reconstruct with the reduced set
	return strip_buffer(buffer, include);
}

bool Optimizer::strip(Buffer &buffer) const
{
	uint32_t counter = 0;
	uint32_t size_begin = buffer.pointer;

	bool changed;
	do {
		changed = strip_once(buffer);
		counter++;
	} while (changed);

	uint32_t size_end = buffer.pointer;
	
	JVL_INFO("ran dead code elimination pass {} times ({} to {})", counter, size_begin, size_end);

	return (size_begin != size_end);
}

// Full passes
void Optimizer::apply(Buffer &buffer) const
{
	distill_types(buffer);
	disolve(buffer);
}

void Optimizer::apply(TrackedBuffer &tracked) const
{
	Buffer &buffer = tracked;
	apply(buffer);

	TrackedBuffer::cache_insert(&tracked);
}

} // namespace jvl::thunder
