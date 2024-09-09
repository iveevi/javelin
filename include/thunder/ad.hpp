#pragma once

#include "buffer.hpp"

namespace jvl::thunder {

void ad_fwd_transform(Buffer &, const Buffer &);
void ad_bwd_transform(Buffer &, const Buffer &);

} // namespace jvl::thunder
