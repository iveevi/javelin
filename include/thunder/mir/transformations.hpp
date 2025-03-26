#pragma once

#include <set>

#include <bestd/tree.hpp>

#include "molecule.hpp"

namespace jvl::thunder::mir {

using Set    = std::set <Index>;
using Linear = std::vector <Ref <Molecule>>;

// Usage addresses for moles
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

// Readdressing methods for moles
template <typename T>
requires bestd::is_variant_component <molecule_base, T>
void readdress(T &, Index a, Index b) {}

void readdress(Operation &, Index, Index);
void readdress(Molecule &, Index, Index);

// Usage and users graphs
bestd::tree <Index, Set> mole_usage(const Block &);
bestd::tree <Index, Set> mole_users(const Block &);

// Legalization passes
Block legalize_storage(const Block &);
Block legalize_calls(const Block &);
Block legalize(const Block &);

// Optimization passes
struct OptimizationPassResult {
	Block block;
	bool changed;
};

OptimizationPassResult optimize_dead_code_elimination_pass(const Block &);
Block optimize_dead_code_elimination(const Block &);

} // namespace jvl::thunder::mir