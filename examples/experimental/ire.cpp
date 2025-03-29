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
		if (addr != -1)
			addr = at(addr);
	}
};

struct Block : Buffer {
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

struct Mapped : std::map <Index, Block> {
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

		return result;
	}
};

void legalize_storage(Buffer &buffer)
{
	auto &em = Emitter::active;

	Mapped mapped(buffer);

	for (size_t i = 0; i < buffer.pointer; i++) {
		auto &atom = buffer.atoms[i];

		if (!atom.is <Store> ())
			continue;

		auto &store = atom.as <Store> ();

		Block block;

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

} // namespace jvl::thunder

func(ftn, i32)(vec2 uv, i32 samples, vec2 resolution)
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

int main()
{
	auto glsl = link(ftn).generate_glsl();
	io::display_lines("FTN", glsl);
	ftn.graphviz("ire.dot");

	// TODO: fix deduplication by doing l-value propogation...
	// TODO: fix optimization around blocks...
	// thunder::optimize(ftn, thunder::OptimizationFlags::eDeadCodeElimination);

	thunder::legalize_storage(ftn);

	auto glsl_opt = link(ftn).generate_glsl();
	io::display_lines("FTN OPT", glsl_opt);
	ftn.graphviz("ire-optimized.dot");
}
