#include "common/io.hpp"
#include "common/logging.hpp"

#include "ire/emitter.hpp"

#include "thunder/legalization.hpp"
#include "thunder/relocation.hpp"

namespace jvl::thunder {

MODULE(legalization);

// Retrieve or construct type of corresponding atom
//     if new atoms are created then the
//     mask will be 0b10, otherwise 0b00
std::pair <Index, uint8_t> type_of(const Buffer &buffer, Index i)
{
	enum : uint8_t {
		eConstructed = 0b10,
		eRetrieved   = 0b00,
	};

	auto &em = ire::Emitter::active;
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

	variant_case(Atom, Operation):
	{
		// Results of operations are necessarily primitives:
		//     since operation overloads exist, it is annoying
		//     to derive the resulting type from the types
		//     of the operands
		auto &qt = buffer.types[i];

		JVL_ASSERT(qt.is_primitive(), "result type of operation is expected to be a primitive");

		auto primitive = qt
			.as <PlainDataType> ()
			.as <PrimitiveType> ();

		auto type = em.emit_type_information(-1, -1, primitive);
		return { type, eConstructed };
	}

	variant_case(Atom, Load):
	{
		auto &qt = buffer.types[i];

		if (qt.is <PlainDataType> ()) {
			auto &pd = qt.as <PlainDataType> ();
			if (auto primitive = pd.get <PrimitiveType> ()) {
				auto type = em.emit_type_information(-1, -1, primitive.value());
				return { type, eConstructed };
			}
		}

		JVL_ABORT("unfinished implementation for load with result type {}:\n{}", qt, atom);
	}

	default:
		break;
	}

	JVL_ABORT("unable to determine the type of atom:\n{}", atom);
}

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
				if (Index(j) == block.value)
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
					if (Index(j) == block.value)
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

///////////////////////////////
// Legalizer implementations //
///////////////////////////////

Legalizer Legalizer::stable;

// TODO: Legalize structure...
void Legalizer::storage(Buffer &buffer)
{
	auto &em = ire::Emitter::active;

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

} // namespace jvl::thunder