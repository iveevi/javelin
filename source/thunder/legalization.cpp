#include "common/io.hpp"
#include "common/logging.hpp"

#include "ire/emitter.hpp"

#include "thunder/legalization.hpp"
#include "thunder/expansion.hpp"

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
	auto &qt = buffer.types[i];
		
	if (qt.is <PlainDataType> ()) {
		auto &pd = qt.as <PlainDataType> ();
		if (auto primitive = pd.get <PrimitiveType> ()) {
			auto type = em.emit_type_information(-1, -1, primitive.value());
			return { type, eConstructed };
		}
	}

	JVL_ABORT("unable to determine the type of atom with qt={}\n{}", qt, atom);
}

///////////////////////////////
// Legalizer implementations //
///////////////////////////////

Legalizer Legalizer::stable;

bool external_refernce(const Atom &atom)
{
	// TODO: only if the parameter is out/inout...
	if (auto ctor = atom.get <Construct> ())
		return ctor->mode == global;
	
	return false;
}

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

		// If the destination reference is out of scope
		// then we cannot legalize it via explicit storage
		auto ridx = buffer.reference_of(store.dst);
		auto &reference = buffer.atoms[ridx];
		JVL_INFO("store instruction: {} (@{})", store.to_assembly_string(), i);
		JVL_INFO("\treference points to: {} (@{})", reference.to_assembly_string(), ridx);
		if (external_refernce(reference)) {
			JVL_INFO("\texternal reference, skipping...");
			continue;
		}

		// Also skip if another store instruction has made progress on this
		if (mapped.contains(store.dst)) {
			JVL_INFO("\tdestination has already been mapped, skipping...");
			continue;
		}

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