#pragma once

#include <set>

#include <bestd/tree.hpp>

#include "molecule.hpp"

namespace jvl::thunder::mir {

using Set = std::set <Index>;

// Usage addresses
template <typename T>
requires bestd::is_variant_component <molecule_base, T>
Set addresses(const T &)
{
	return {};
}

Set addresses(const Type &);
Set addresses(const Construct &);
Set addresses(const Operation &);
Set addresses(const Intrinsic &);
Set addresses(const Store &);
Set addresses(const Storage &);
Set addresses(const Return &);

// Usage and users graphs
bestd::tree <Index, Set> mole_usage(const Block &);
bestd::tree <Index, Set> mole_users(const Block &);

} // namespace jvl::thunder::mir