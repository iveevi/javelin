#pragma once

namespace jvl::profiles {

struct glsl_version {
	const char *str;
} static glsl_330("330"),
	 glsl_410("410"),
	 glsl_420("420"),
	 glsl_450("450"),
	 glsl_460("460");

struct cpp_standard {
	// Nothing here...
};

// TODO: cuda, spirv

} // namespace jvl::profiles
