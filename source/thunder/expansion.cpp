#include <stack>

#include "common/io.hpp"
#include "common/logging.hpp"

#include "thunder/expansion.hpp"
#include "thunder/relocation.hpp"

namespace jvl::thunder {

MODULE(expansion);

// Expansion buffer methods
void Expansion::returns(Index i)
{
	value = i;
}

void Expansion::track(Index i, uint8_t mask)
{
	locals.emplace(i, mask);
}

// Mapped collection methods
Mapped::Mapped(Buffer &base_) : base(base_) {}

void Mapped::display() const
{
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

Buffer Mapped::stitch() const
{
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

} // namespace jvl::thunder