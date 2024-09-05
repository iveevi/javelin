#pragma once

#include <functional>
#include <vector>

#include "../profiles/targets.hpp"
#include "atom.hpp"

namespace jvl::thunder {

struct Linkage;

class Kernel {
	std::string synthesize_glsl(const std::string &);
	std::string synthesize_cplusplus();
public:
	// Name of the kernel, usually not set by the user
	std::string name;

	// At this point the IR atoms are unlikely to change
	std::vector <Atom> atoms;

	// Possible kernel flags, which enable certain paths of synthesis
	enum Kind {
		eNone            = 0x0,
		eVertexShader    = 0x1,
		eFragmentShader  = 0x10,
		eCallable        = 0x100,
		eAll             = eVertexShader | eFragmentShader | eCallable,
	} kind;

	// Must specify the type by the time of construction
	Kernel(Kind k) : name("kernel"), kind(k) {}

	// Get linkage model for the kernel
	Linkage linkage() const;

	// Synthesizing targets
	std::string synthesize(const profiles::glsl_version version) {
		return synthesize_glsl(version.str);
	}

	std::string synthesize(const profiles::cpp_standard standard) {
		return synthesize_cplusplus();
	}

	// Checking for capabilities
	[[gnu::always_inline]]
	inline bool is_compatible(Kind k) const {
		return (k & kind) == k;
	}

	// Printing the stored IR
	void dump() const;
};

} // namespace jvl::thunder
