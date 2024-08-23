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
	// Element of a scene
	struct Object {
		// Identifier for the object
		std::string name;

		// At most one geometry instance per object
		std::optional <Mesh> geometry;

		// An object can have multiple materials which
		// are references from the geometry
		std::vector <Material> materials;
	};

	std::vector <Object> objects;

	void add(const engine::ImportedAsset &);

	void write(const std::filesystem::path &);
};

} // namespace jvl::core
