#pragma once

namespace jvl::profiles {

struct glsl_version {
	const char *str;
} static glsl_330("330"),
	 glsl_450("450"),
	 glsl_460("460");

struct cpp_standard {
	// Nothing here...
} static cplusplus_11,
	 cplusplus_14,
	 cplusplus_17,
	 cplusplus_20;

struct cuda_api {
	// Nothing here...
} static cuda_8,
	 cuda_9,
	 cuda_10,
	 cuda_11,
	 cuda_12;

} // namespace jvl::profiles
