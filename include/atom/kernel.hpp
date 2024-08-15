#pragma once

#include <vector>

#include "../profiles/targets.hpp"
#include "atom.hpp"

namespace jvl::atom {

class Kernel {
	std::string synthesize_glsl(const std::string &);
	std::string synthesize_cplusplus();
public:
	// Name of the kernel, usually not set by the user
	std::string name;

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

	// At this point the IR atoms are unlikely to change
	std::vector <atom::General> atoms;
	std::unordered_set <atom::index_t> used;
	std::unordered_set <atom::index_t> synthesized;

	// Synthesizing targets
	std::string synthesize(const profiles::glsl_version version) {
		return synthesize_glsl(version.str);
	}

	// TODO: potentially pass a name for the function
	std::string synthesize(const profiles::cpp_standard standard) {
		return synthesize_cplusplus();
	}

	[[gnu::always_inline]]
	inline bool is_compatible(Kind k) {
		return (k & kind) == k;
	}

	// Printing the stored IR
	void dump();
};

} // namespace jvl::atom
