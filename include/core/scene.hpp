#pragma once

#include <filesystem>
#include <vector>

#include "core/mesh.hpp"
#include "core/material.hpp"
#include "engine/imported_asset.hpp"

// Forward declarations
namespace jvl::engine {

struct ImportedAsset;

}

// Primary declaration
namespace jvl::core {

struct Scene {
	std::vector <Mesh> meshes;
	std::vector <Material> materials;

	void add(const engine::ImportedAsset &);

	void write(const std::filesystem::path &);
};

} // namespace jvl::core
