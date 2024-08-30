#pragma once

#include "atom.hpp"
#include "../ire/scratch.hpp"

namespace jvl::thunder {

using usage_list = std::vector <index_t>;
using usage_graph = std::vector <usage_list>;

// Atom usage dependency retrieval
usage_list usage(const std::vector <Atom> &, index_t);
usage_list usage(const ire::Scratch &, index_t);
usage_graph usage(const ire::Scratch &);

// Optimization transformation passes
void opt_transform_compact(ire::Scratch &);
void opt_transform_constructor_elision(ire::Scratch &);
void opt_transform_dead_code_elimination(ire::Scratch &);

} // namespace jvl::thunder