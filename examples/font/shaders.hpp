#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

// Actual shader programs
extern Procedure <void> vertex;
extern Procedure <void> fragment;

extern void shader_debug();
