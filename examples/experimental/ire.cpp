// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <queue>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <common/io.hpp>

#include <ire.hpp>

#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: unsized buffers in aggregates... only for buffers
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: shadertoy and fonts example
// TODO: get line numbers for each invocation if possible?
// using source location if available

// // Type safe options: shaders as functors...
// struct Vertex {
// 	gl_Position_t gl_Position;

// 	void $return() {

// 	}

// 	virtual void operator()() = 0;
// };

// template <generic_or_void R>
// struct Method {
// 	// Restrict certain operations for type checking reasons...
// 	void $return(const R &) {

// 	}
// };

// struct Shader : Vertex {
// 	void operator()() override {

// 	}
// };

namespace jvl::thunder {

// Since AIR is linear we only need a range to specify any scope
struct Scope {
	Index begin;
	Index end;
};

struct Block : Scope {};

// Retrieve or construct type of corresponding atom
//     if new atoms are created then the
//     mask will be 0b10, otherwise 0b00
std::pair <Index, uint8_t> type_of(const Buffer &buffer, Index i)
{
	enum : uint8_t {
		eConstructed = 0b10,
		eRetrieved   = 0b00,
	};

	auto &em = Emitter::active;
	auto &atom = buffer.atoms[i];

	switch (atom.index()) {
	
	variant_case(Atom, Primitive):
	{
		auto &primitive = atom.as <Primitive> ();
		auto type = em.emit_type_information(-1, -1, primitive.type);
		return { type, eConstructed };
	}

	variant_case(Atom, Construct):
	{
		auto &ctor = atom.as <Construct> ();
		return { ctor.type, eRetrieved };
	}

	default:
		break;
	}

	JVL_ABORT("unable to determine the type of atom:\n{}", atom);
}

struct Relocation : std::map <Index, Index> {
	void apply(Index &addr) const {
		auto it = find(addr);
		if (addr != -1 && it != end())
			addr = it->second;
	}

	void apply(Atom &atom) const {
		auto addrs = atom.addresses();

		apply(addrs.a0);
		apply(addrs.a1);
	}

	void apply(Buffer &buffer) const {
		for (size_t i = 0; i < buffer.pointer; i++)
			apply(buffer.atoms[i]);
	}
};

struct Expansion : Buffer {
	Index value;

	// local index -> mask of local addresses
	//     mask = 0bXY
	//     X is mask (on=local) for 1st arg
	//     Y is mask (on=local) for 2nd arg 
	std::map <Index, uint8_t> locals;

	void returns(Index i) {
		value = i;
	}

	void track(Index i, uint8_t mask = 0b11) {
		locals.emplace(i, mask);
	}
};

struct Mapped : std::map <Index, Expansion> {
	Buffer &base;

	Mapped(Buffer &base_) : base(base_) {}

	void display() const {
		io::header("MAPPED BLOCKS", 50);

		for (auto &[i, block] : *this) {
			fmt::println("{} becomes:", base.atoms[i].to_assembly_string());
			for (size_t j = 0; j < block.pointer; j++) {
				uint8_t mask = 0b00;
				if (block.locals.contains(j))
					mask = block.locals.at(j);

				std::string val = " ";
				if (j == block.value)
					val = "*";

				fmt::println("\t{}[{:02b}] {}", val, mask, block.atoms[j].to_assembly_string());
			}
		}
	}

	Buffer stitch() const {
		Buffer result;

		// index in buffer -> index in result
		//     if the index is mapped then the result index
		//     corresponds to the return value of the block
		Relocation returns;

		// Branch instructions which need <failto> resolution
		std::stack <Index> branches;

		for (size_t i = 0; i < base.pointer; i++) {
			auto atom = base.atoms[i];
			auto addrs = atom.addresses();

			if (contains(i)) {
				const auto &block = at(i);
				const auto &locals = block.locals;

				// local -> index in result
				Relocation real;

				for (size_t j = 0; j < block.pointer; j++) {
					auto atom = block.atoms[j];
					auto addrs = atom.addresses();

					// Apply necessary remapping...
					uint8_t mask = 0b00;
					if (locals.contains(j))
						mask = locals.at(j);
					
					// ...to the first address
					if ((mask & 0b10) == 0b10)
						real.apply(addrs.a0);
					else if (addrs.a0 != -1)
						returns.apply(addrs.a0);
					
					// ... to the second address
					if ((mask & 0b01) == 0b01)
						real.apply(addrs.a1);
					else if (addrs.a1 != -1)
						returns.apply(addrs.a1);

					// Generate instruction
					auto k = result.emit(atom);

					real.emplace(j, k);
					if (j == block.value)
						returns.emplace(i, k);
				}
			} else if (contains(addrs.a0) || contains(addrs.a1)) {
				returns.apply(addrs.a0);
				returns.apply(addrs.a1);

				auto k = result.emit(atom);

				returns.emplace(i, k);
			} else {
				returns.apply(addrs.a0);

				if (atom.is <Branch> ()) {
					addrs.a1 = -1;
				} else {
					returns.apply(addrs.a1);
				}

				auto k = result.emit(atom);
				
				returns.emplace(i, k);

				if (atom.is <Branch> ()) {
					auto &current = atom.as <Branch> ();

					if (current.kind == control_flow_end) {
						auto idx = branches.top();
						branches.pop();

						result.atoms[idx].as <Branch> ().failto = k;
					} else {
						branches.emplace(k);
					}
				}
			}
		}

		JVL_ASSERT(branches.empty(), "failed to fully resolve branches ({} left)", branches.size());

		// TODO: transfer decorations...

		return result;
	}
};


struct Legalizer {
	// TODO: flags...

	void storage(Buffer &);
};

// TODO: Legalize structure...
void Legalizer::storage(Buffer &buffer)
{
	auto &em = Emitter::active;

	Mapped mapped(buffer);

	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];

		if (!atom.is <Store> ())
			continue;

		auto &store = atom.as <Store> ();

		Expansion block;

		em.push(block, false);
		{
			auto [s0, s1_mask] = type_of(buffer, store.dst);
			auto s1 = em.emit_storage(s0);
			auto s2 = em.emit(buffer.atoms[store.dst]);
			auto s3 = em.emit_store(s1, s2);

			block.track(s1, s1_mask);
			block.track(s3);

			// Ultimately, the new value is the storage
			block.returns(s1);
		}
		em.pop();

		mapped.emplace(store.dst, block);
	}

	mapped.display();

	buffer = mapped.stitch();
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

struct Optimizer {
	using HashMap = std::map <uint64_t, Index>;

	OptimizationFlags flags;

	// Instruction distillation (deduplication)
	uint32_t distill_types(Buffer &) const;

	void distill(Buffer &buffer) const {

	}

	// Instruction disolving (removal and elision)
	void disolve_casting(Relocation &relocation, const Buffer &buffer, const Intrinsic &intr) const {
		fmt::println("casting instruction: {}", intr.to_assembly_string());

		auto list = buffer.atoms[intr.args].as <List> ();
		auto qt = buffer.types[list.item];

		fmt::println("\ttype is {}", qt);
	}

	void disolve_constructor(Relocation &relocation, const Buffer &buffer, const Construct &ctor, Index i) const {
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

	void disolve(Buffer &buffer) const {
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

	// Dead code elimination
	bool strip_buffer(Buffer &buffer, const std::vector <bool> &include) const {
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

	bool strip_once(Buffer &buffer) const {
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

	bool strip(Buffer &buffer) const {
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
};

uint32_t Optimizer::distill_types(Buffer &buffer) const
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

} // namespace jvl::thunder

func(i32, ftn)(i32 samples)
{
	i32 count;

	auto it = range <i32> (0, samples, 1);
	
	auto i = $for(it);
	{
		auto j = $for(it);
		{
			count += i32(i != j);
		}
		$end();
	}
	$end();

	$return(count);
};

// TODO: legalize stores through a storage atom...

// TODO: partial specialization of arguments
// TODO: refactor to func(R, name, compile-time args...)

int main()
{
	auto glsl = link(ftn).generate_glsl();
	io::display_lines("FTN", glsl);
	ftn.graphviz("ire.dot");

	// TODO: fix deduplication by doing l-value propogation...
	// TODO: fix optimization around blocks...
	// thunder::optimize(ftn, thunder::OptimizationFlags::eDeadCodeElimination);

	thunder::Legalizer().storage(ftn);

	auto glsl_leg = link(ftn).generate_glsl();
	io::display_lines("FTN LEGALIZED", glsl_leg);
	ftn.graphviz("ire-legalized.dot");

	auto optimizer = thunder::Optimizer();
	optimizer.distill_types(ftn);
	optimizer.strip(ftn);
	optimizer.distill_types(ftn);
	optimizer.strip(ftn);
	optimizer.disolve(ftn);
	optimizer.strip(ftn);
	
	auto glsl_opt = link(ftn).generate_glsl();
	io::display_lines("FTN OPTIMIZED", glsl_opt);
	ftn.graphviz("ire-optimized.dot");
}
