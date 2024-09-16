#pragma once

#include "buffer.hpp"
#include "../profiles/targets.hpp"

namespace jvl::thunder {

struct Linkage;

struct Kernel : Buffer {
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
	Kernel(const Buffer &buffer, Kind k) : Buffer(buffer), name("kernel"), kind(k) {}

	// Get linkage model for the kernel
	Linkage linkage() const;

	// Synthesizing targets
	std::string compile(const profiles::glsl_version &);
	std::string compile(const profiles::cpp_standard &);

	// Checking for capabilities
	[[gnu::always_inline]]
	inline bool compatible_with(Kind k) const {
		return (k & kind) == k;
	}

	// Printing the stored IR
	void dump() const;
};

} // namespace jvl::thunder
