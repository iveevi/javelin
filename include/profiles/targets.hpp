#pragma once

namespace jvl::profiles {

struct glsl_version {
	const char *version;

	bool explicit_location;
} static glsl_330("330", false),
	 glsl_400("400", false),
	 glsl_410("410", true),
	 glsl_420("420", true),
	 glsl_430("430", true),
	 glsl_440("440", true),
	 glsl_450("450", true),
	 glsl_460("460", true);

struct cpp_standard {
	// Nothing here...
};

// TODO: cuda, spirv

} // namespace jvl::profiles
