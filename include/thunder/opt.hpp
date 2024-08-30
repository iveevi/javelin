#pragma once

#include "atom.hpp"
#include "../ire/scratch.hpp"

namespace jvl::thunder {

using usage_list = std::vector <index_t>;
using usage_graph = std::vector <usage_list>;

usage_list usage(const std::vector <Atom> &, index_t);
usage_list usage(const ire::Scratch &, index_t);
usage_graph usage(const ire::Scratch &);

} // namespace jvl::thunder