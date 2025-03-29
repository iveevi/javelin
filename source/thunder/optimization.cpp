#include <queue>
#include <unordered_set>

#include "common/logging.hpp"

#include "thunder/optimization.hpp"

namespace jvl::thunder {

MODULE(optimization);

void refine_relocation(reindex <Index> &relocation)
{
	for (auto &[k, v] : relocation) {
		auto pv = v;

		do {
			pv = v;
			relocation(v);
		} while (pv != v);
	}
}

void buffer_relocation(Buffer &result, const reindex<Index> &relocation)
{
	for (size_t i = 0; i < result.pointer; i++)
		result.atoms[i].reindex(relocation);
}

bool optimize_dead_code_elimination_iteration(Buffer &result)
{
	UsageGraph graph = usage(result);

	// Reversed usage graph
	// TODO: at most 3 refs per atom, so use an array?
	UsageGraph reversed(graph.size());
	for (size_t i = 0; i < graph.size(); i++) {
		for (Index j : graph[i])
			reversed[j].insert(i);
	}

	// Configure checking queue and inclusion mask
	std::vector <bool> include(result.pointer, true);
	
	std::queue <Index> check_list;
	for (size_t i = 0; i < result.pointer; i++)
		check_list.push(i);

	// Keep checking as long as something got erased
	while (check_list.size()) {
		std::unordered_set <Index> erasure;

		while (check_list.size()) {
			Index i = check_list.front();
			check_list.pop();
			
			if (graph[i].empty() && !result.marked.contains(i)) {
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
	Index pointer = 0;

	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++) {
		if (include[i])
			relocation[i] = pointer++;
	}

	Buffer doubled;
	for (size_t i = 0; i < result.pointer; i++) {
		if (relocation.contains(i)) {
			result.atoms[i].reindex(relocation);
			doubled.emit(result.atoms[i]);
		}
	}

	// Transfer decorations
	doubled.decorations.all = result.decorations.all;

	for (auto &[i, j] : result.decorations.used) {
		auto k = relocation[i];
		doubled.decorations.used[k] = j;
	}
	
	for (auto &i : result.decorations.phantom) {
		auto k = relocation[i];
		doubled.decorations.phantom.insert(k);
	}

	std::swap(result, doubled);

	// Change happened only if there is a difference in size
	return (result.pointer != doubled.pointer);
}

bool optimize_dead_code_elimination(Buffer &result)
{
	uint32_t counter = 0;
	uint32_t size_begin = result.pointer;

	bool changed;
	do {
		changed = optimize_dead_code_elimination_iteration(result);
		counter++;
	} while (changed);

	uint32_t size_end = result.pointer;
	
	JVL_INFO("ran dead code elimination pass {} times ({} to {})", counter, size_begin, size_end);

	return size_begin != size_end;
}

uint32_t optimize_deduplicate_iteration(Buffer &result)
{
	static_assert(sizeof(Atom) == sizeof(uint64_t));
	
	uint32_t counter = 0;

	// Find which instructions to lock based on l-value requirements and etc.
	std::set <Index> locked;

	for (size_t i = 0; i < result.pointer; i++) {
		auto &atom = result.atoms[i];

		// TODO: what about intrinsics and calls?
		if (atom.is <Store> ()) {
			Index dst = atom.as <Store> ().dst;
			Index real = result.reference_of(dst);
			locked.insert(real);
		}
	}

	// Each atom converted to a 64-bit integer
	std::map <uint64_t, Index> existing;

	// TODO: move outside...
	auto unique = [&](Index i) -> Index {
		auto &atom = result.atoms[i];

		// Handle exceptions
		bool exempt = false;
		exempt |= atom.is <Store> ();
		exempt |= atom.is <Call> ();
		exempt |= atom.is <Branch> ();
		exempt |= locked.contains(i);

		if (exempt)
			return i;

		auto &hash = reinterpret_cast <uint64_t &> (atom);

		auto it = existing.find(hash);
		if (it != existing.end()) {
			if (i != it->second) {
				counter++;
				result.marked.erase(i);
			}

			return it->second;
		}

		existing[hash] = i;

		return i;
	};

	for (size_t i = 0; i < result.pointer; i++) {
		auto addresses = result.atoms[i].addresses();
		if (addresses.a0 != -1)
			addresses.a0 = unique(addresses.a0);
		if (addresses.a1 != -1)
			addresses.a1 = unique(addresses.a1);
	}

	return counter;
}

bool optimize_deduplicate(Buffer &result)
{
	uint32_t counter = 0;
	uint32_t unlinked = 0;

	bool changed;

	do {
		auto c = optimize_deduplicate_iteration(result);
		unlinked += c;
		changed = (c > 0);
		counter++;
	} while (changed);

	JVL_INFO("ran deduplication pass {} times (unlinked {})", counter, unlinked);

	return (counter > 1);
}

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

bool optimize_casting_elision(Buffer &result)
{
	uint32_t counter = 0;

	auto graph = usage(result);
	
	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++) {
		auto &atom = result.atoms[i];
		auto &oqt = result.types[i];

		auto list = List();

		if (atom.is <Construct> ()) {
			// Constructors with one argument of the same type
			auto &ctor = atom.as <Construct> ();
			if (ctor.mode != normal || ctor.args == -1)
				continue;

			auto &args = result.atoms[ctor.args];
			JVL_ASSERT(args.is <List> (), "constructor arguments ({}) should be a list chain:\n{}",
				ctor.to_assembly_string(), args);

			list = args.as <List> ();
		} else if (atom.is <Intrinsic> ()) {
			auto &intr = atom.as <Intrinsic> ();
			if (!casting_intrinsic(intr))
				continue;

			// Casting intrinsics with the same type
			auto &args = result.atoms[intr.args];
			JVL_ASSERT(args.is <List> (), "intrinsic arguments ({}) should be a list chain:\n{}",
				intr.to_assembly_string(), args);

			list = args.as <List> ();
		} else {
			continue;
		}

		// Only handle singletons
		if (list.next != -1)
			continue;
		
		auto &aqt = result.types[list.item];
		if (oqt == aqt) {
			relocation[i] = list.item;
			result.marked.erase(i);
			counter++;
		}
	}

	refine_relocation(relocation);
	buffer_relocation(result, relocation);

	JVL_INFO("disolved {} casting instructions", counter);

	return (counter > 0);
}

// Trimming unnecessary store instructions
bool optimize_store_elision(Buffer &result)
{
	uint32_t counter = 0;
	
	auto graph = usage(result);
	
	reindex <Index> relocation;
	for (size_t i = 0; i < result.pointer; i++) {
		auto &atom = result.atoms[i];
		if (!atom.is <Store> ())
			continue;

		auto &store = atom.as <Store> ();
		
		// Writing directly to mutable references
		auto &dst = result.atoms[store.dst];

		if (auto ctor = dst.get <Construct> ()) {
			if (ctor->mode == global)
				continue;
		}

		// Collect all extended uses...
		bool write_before_use = false;

		std::queue <Index> check;
		check.push(store.dst);

		// TODO: flag for verbose... (in optimizer class)
		fmt::println("considering store: {}", store.to_assembly_string());

		std::set <Index> addresses;
		while (!check.empty()) {
			auto j = check.front();
			check.pop();

			auto &atom = result.atoms[j];

			// Loading references does not count towards use
			bool addressable = false
				|| atom.is <Swizzle> ()
				|| atom.is <Load> ()
				|| atom.is <ArrayAccess> ();

			if (j == store.dst || addressable) {
				// Load the uses of the addresses
				fmt::println("\tadding addressable instruction: {}", atom.to_assembly_string());
				for (auto jj : graph[j]) {
					if (addresses.contains(jj))
						continue;

					check.push(jj);
				}
			} else {
				// Check for earlier uses
				fmt::println("\tnon-address instruction: {}", atom.to_assembly_string());
				if (j != store.dst && j < Index(i)) {
					fmt::println("\tearler use found:");
					fmt::println("\t\t{}", result.atoms[j].to_assembly_string());
					write_before_use = true;
					break;
				}
			}
		}

		if (!write_before_use) {
			auto &src = result.atoms[store.src];
			fmt::println("\tsingle store will be skipped:");
			fmt::println("\t\tdst: {}", dst.to_assembly_string());
			fmt::println("\t\tsrc: {}", src.to_assembly_string());

			relocation[store.dst] = store.src;

			result.marked.erase(i);
			result.marked.erase(store.dst);
			counter++;
		}
	}

	refine_relocation(relocation);
	buffer_relocation(result, relocation);
	
	return (counter > 0);
}

// Display different stages
static constexpr size_t NATOMS = Atom::type_index <Return> () + 1;

std::array <uint32_t, NATOMS> composition(const Buffer &buffer)
{
	std::array <uint32_t, NATOMS> result;

	result.fill(0);
	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];
		result[atom.index()]++;
	}

	return result;
}

void optimize_impl(Buffer &result, const OptimizationFlags flags)
{
	uint32_t counter = 0;

	bool changed;

	auto composition_before = composition(result);

	do {
		changed = false;

		if (has(flags, OptimizationFlags::eDeadCodeElimination))
			changed |= optimize_dead_code_elimination(result);

		if (has(flags, OptimizationFlags::eDeduplication))
			changed |= optimize_deduplicate(result);

		if (has(flags, OptimizationFlags::eCastingElision))
			changed |= optimize_casting_elision(result);

		if (has(flags, OptimizationFlags::eStoreElision))
			changed |= optimize_store_elision(result);

		counter++;
	} while (changed);
	
	auto composition_after = composition(result);

	JVL_INFO("ran full optimization pass {} times", counter);
	JVL_INFO("summary of changes:");
	
	// Summary of changes
	constexpr std::array <const char *const, NATOMS> strings {
		"Qualifier",
		"TypeInformation",
		"Primitive",
		"Swizzle",
		"Operation",
		"Intrinsic",
		"List",
		"Construct",
		"Call",
		"Storage",
		"Store",
		"Load",
		"ArrayAccess",
		"Branch",
		"Returns",
	};

	for (size_t i = 0; i < NATOMS; i++) {
		JVL_INFO("\t{:>15}: {:4d} -> {:4d}",
			strings[i],
			composition_before[i],
			composition_after[i]);
	}
}

void optimize(Buffer &result, const OptimizationFlags flags)
{
	JVL_STAGE();
	optimize_impl(result, flags);
}

void optimize(TrackedBuffer &result, const OptimizationFlags flags)
{
	JVL_STAGE_NAMED(fmt::format("optimization of '{}.{}'", result.name, result.cid));
	optimize_impl(result, flags);

	// Update the cache entry
	TrackedBuffer::cache_insert(&result);
}

} // namespace jvl::thunder
