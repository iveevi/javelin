#pragma once

#include <filesystem>
#include <vector>

#include "imported_mesh.hpp"
#include "material.hpp"

// Constructed when loading a scene from a path
struct ImportedAsset {
	std::vector <std::string> names;
	std::vector <ImportedMesh> geometries;
	std::vector <jvl::core::Material> materials;
	std::filesystem::path path;

	// TODO: materials as well

	static std::optional <ImportedAsset> from(const std::filesystem::path &);
};