#pragma once

#include <filesystem>
#include <vector>

#include "mesh.hpp"
#include "material.hpp"

namespace jvl::core {

// Constructed when loading a scene from a path
struct Preset {
	std::vector <Mesh> geometry;
	std::vector <Material> materials;
	std::filesystem::path path;

	// TODO: materials as well

	static std::optional <Preset> from(const std::filesystem::path &);
};

} // namespace jvl::core
