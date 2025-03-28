#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

// Actual shader programs
extern Procedure <void> vertex;
extern void fragment(int32_t);

extern void shader_debug();
