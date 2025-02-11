#pragma once

#include <filesystem>
#include <vector>

#include "imported_mesh.hpp"
#include "imported_material.hpp"

// Constructed when loading a scene from a path
struct ImportedAsset {
	std::vector <std::string> names;
	std::vector <ImportedMesh> geometries;
	std::vector <ImportedMaterial> materials;
	std::filesystem::path path;

	static std::optional <ImportedAsset> from(const std::filesystem::path &);
};