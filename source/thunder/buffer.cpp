#include "core/logging.hpp"

#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(buffer);

Buffer::Buffer() : pointer(0), atoms(4), types(4) {}

Index Buffer::emit(const Atom &atom, bool enable_classification)
{
	if (pointer >= atoms.size()) {
		atoms.resize((atoms.size() << 1));
		types.resize(atoms.size());
	}

	JVL_ASSERT(pointer < atoms.size(),
		"scratch buffer pointer is out of bounds ({} >= {})",
		pointer, atoms.size());

	atoms[pointer] = atom;
	if (enable_classification) {
		types[pointer] = semalz(pointer);
		mark(pointer);
	}

	return pointer++;
}

void Buffer::clear()
{
	pointer = 0;
	atoms.clear();
	atoms.resize(4);
}

void Buffer::dump() const
{
	for (size_t i = 0; i < pointer; i++) {
		fmt::println("   [{:4d}] {:55}"
			"\n          :: type: {:20}"
			"\n          :: decorations: ({}{})",
			i, atoms[i],
			fmt::format(fmt::emphasis::underline, "{}", types[i]),
			synthesized.contains(i) ? 's' : '-',
			used_decorations.contains(i) ? 't' : '-');
	}

	// Decorations
	fmt::println("   [   X] {}", fmt::format(fmt::emphasis::bold, "{}", "DECORATIONS"));

	for (auto &[i, th] : decorations) {
		fmt::println("          :: {} (#{})", fmt::format(fmt::emphasis::underline, "{}", th.name), i);
		for (auto &s : th.fields)
			fmt::println("             {}", s);
	}
}

// Utility methods
Atom &Buffer::operator[](size_t i)
{
	return atoms[i];
}

const Atom &Buffer::operator[](size_t i) const
{
	return atoms[i];
}

std::vector <Index> Buffer::expand_list(Index i) const
{
	std::vector <Index> args;
	while (i != -1) {
		auto &atom = atoms[i];
		JVL_ASSERT_PLAIN(atom.is <List> ());

		List list = atom.as <List> ();
		args.push_back(list.item);

		i = list.next;
	}

	return args;
}

std::vector <QualifiedType> Buffer::expand_list_types(Index i) const
{
	std::vector <QualifiedType> args;
	while (i != -1) {
		auto &atom = atoms[i];

		JVL_ASSERT(atom.is <List> (), "unexpected atom in list chain:\n{}", atom);

		List list = atom.as <List> ();
		i = list.next;

		args.push_back(types[list.item]);
	}

	return args;
}

} // namespace jvl::thunder
