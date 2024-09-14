#include "thunder/atom.hpp"

namespace jvl::thunder {

template <atom_instruction A, atom_instruction B>
static constexpr bool compare(const A &a, const B &b)
{
	return false;
}

template <atom_instruction A>
static bool compare(const A &a, const A &b)
{
	return a == b;
}

bool Atom::operator==(const Atom &other) const
{
	auto ftn = [](auto a, auto b) { return compare(a, b); };
	return std::visit(ftn, *this, other);
}

} // namespace jvl::thunder