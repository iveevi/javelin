#pragma once

#include <vector>

#include "../profiles/targets.hpp"
#include "atom.hpp"

namespace jvl::atom {

class Kernel {
	std::string synthesize_glsl(const std::string &);
	std::string synthesize_cplusplus();
public:
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

	// Printing the stored IR
	void dump();
};

} // namespace jvl::atom
