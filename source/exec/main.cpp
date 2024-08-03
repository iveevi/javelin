#include <cassert>
#include <filesystem>

#include <fmt/printf.h>
#include <fmt/std.h>

#include "core/preset.hpp"

namespace jvl {

struct Kernel {
	enum {
		eCPU,
		eCUDA,
		eVulkan,
		eOpenGL
	} backend;
};

}

#include "core/transform.hpp"
#include "core/aperature.hpp"

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	jvl::core::Preset::from(path);
}
