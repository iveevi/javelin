#pragma once

#include "scratch.hpp"

namespace jvl::thunder {

void ad_fwd_transform(Scratch &, const Scratch &);
void ad_bwd_transform(Scratch &, const Scratch &);

} // namespace jvl::thunder
