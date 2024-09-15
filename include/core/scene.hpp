#pragma once

#include <filesystem>
#include <vector>
#include <set>
#include <map>

#include "mesh.hpp"
#include "material.hpp"
#include "messaging.hpp"

// Forward declarations
namespace jvl::engine {

struct ImportedAsset;

}

// Primary declaration
namespace jvl::core {

struct Scene {
	// Element of a scene
	struct Object {
		// Global ID
		UUID uuid;

		// Identifier for the object
		std::string name;

		// At most one geometry instance per object
		std::optional <Mesh> geometry;

		// An object can have multiple materials which
		// are references from the geometry
		std::vector <Material> materials;

		// Children
		std::set <int64_t> children;

		// Global ID
		int64_t id() const {
			return uuid.global;
		}
	};

	std::set <int64_t> root;
	std::map <int64_t, Object> objects;

	void add_root(const Object &);
	void add(const Object &);
	void add(const engine::ImportedAsset &);

	Object &operator[](int64_t);
	const Object &operator[](int64_t) const;

	void write(const std::filesystem::path &);
};

} // namespace jvl::core
