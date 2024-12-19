#pragma once

#include <filesystem>
#include <vector>

#include "mesh.hpp"
#include "material.hpp"

namespace jvl::engine {

// Constructed when loading a scene from a path
struct ImportedAsset {
	std::vector <std::string> names;
	std::vector <core::Mesh> geometries;
	std::vector <core::Material> materials;
	std::filesystem::path path;

	// TODO: materials as well

	static std::optional <ImportedAsset> from(const std::filesystem::path &);
};

} // namespace jvl::core
