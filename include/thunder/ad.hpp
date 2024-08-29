#pragma once

namespace jvl::ire {

struct Scratch;

} // namespace jvl::ire

namespace jvl::thunder {

void ad_fwd_transform(ire::Scratch &, const ire::Scratch &);
void ad_bwd_transform(ire::Scratch &, const ire::Scratch &);

} // namespace jvl::thunder