#pragma once

#include <filesystem>
#include <vector>

#include "mesh.hpp"

namespace jvl::core {

// Constructed when loading a scene from a path
struct Preset {
	std::vector<Mesh> geometry;
	std::filesystem::path path;

	// TODO: materials as well

	static std::optional<Preset> from(const std::filesystem::path &);
};

} // namespace jvl::core
