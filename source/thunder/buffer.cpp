#include "common/logging.hpp"

#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(buffer);

static constexpr size_t BUFFER_STARTUP_SIZE = 64;

// TODO: provide parameters to control startup size...
Buffer::Buffer()
	: pointer(0),
	atoms(BUFFER_STARTUP_SIZE),
	types(BUFFER_STARTUP_SIZE) {}

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

// Debugging utilities
void Buffer::write(std::ofstream &file) const
{
	size_t synthesized_count = marked.size();
	file.write(reinterpret_cast <const char *> (&synthesized_count), sizeof(size_t));

	std::vector <Index> synthesized_vector(marked.begin(), marked.end());
	file.write(reinterpret_cast <const char *> (synthesized_vector.data()), synthesized_count * sizeof(Index));

	file.write(reinterpret_cast <const char *> (&pointer), sizeof(size_t));
	file.write(reinterpret_cast <const char *> (atoms.data()), pointer * sizeof(Atom));
	file.write(reinterpret_cast <const char *> (types.data()), pointer * sizeof(QualifiedType));

	// TODO: write decorations
}

std::string Buffer::to_string_assembly() const
{
	std::string result;

	for (size_t i = 0; i < pointer; i++) {
		std::string s = fmt::format("%{}", i);
		result += fmt::format("{:>5} = {}\n", s, atoms[i].to_assembly_string());
	}

	return result;
}

std::string Buffer::to_string_pretty() const
{
	std::string result;

	for (size_t i = 0; i < pointer; i++) {
		result += fmt::format("   [{:4d}] {:55}"
			"\n          :: type: {:20}"
			"\n          :: decorations: ({}{}{})\n",
			i, atoms[i],
			fmt::format(fmt::emphasis::underline, "{}", types[i]),
			marked.contains(i) ? 's' : '-',
			decorations.used.contains(i) ? 't' : '-',
			decorations.phantom.contains(i) ? 'p' : '-');
	}

	// Decorations
	result += fmt::format("   [   X] {}\n", fmt::format(fmt::emphasis::bold, "{}", "DECORATIONS"));

	for (auto &[i, th] : decorations.all) {
		result += fmt::format("          :: {} (#{})\n",
			fmt::format(fmt::emphasis::underline, "{}", th.name), i);

		for (auto &s : th.fields)
			result += fmt::format("             {}\n", s);
	}

	return result;
}

void Buffer::display_assembly() const
{
	for (size_t i = 0; i < pointer; i++) {
		std::string s = fmt::format("%{}", i);
		fmt::println("{:>5} = {}", s, atoms[i].to_assembly_string());
	}
}

void Buffer::display_pretty() const
{
	for (size_t i = 0; i < pointer; i++) {
		fmt::println("   [{:4d}] {:55}"
			"\n          :: type: {:20}"
			"\n          :: decorations: ({}{}{})",
			i, atoms[i],
			fmt::format(fmt::emphasis::underline, "{}", types[i]),
			marked.contains(i) ? 's' : '-',
			decorations.used.contains(i) ? 't' : '-',
			decorations.phantom.contains(i) ? 'p' : '-');
	}

	// Decorations
	fmt::println("   [   X] {}", fmt::format(fmt::emphasis::bold, "{}", "DECORATIONS"));

	for (auto &[i, th] : decorations.all) {
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
