#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <set>
#include <vector>

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
	// Archetype layout
	std::vector <Mesh> geometries;
	// std::vector <Material> materials;
	Archetype <Material> materials;

	// Element of a scene
	struct Object : Unique {
		// TODO: dirty flag (top level)

		// Origin scene
		Scene *const reference;

		// Identifier for the object
		std::string name;

		// At most one geometry instance per object
		int64_t geometry;

		// An object can have multiple materials which
		// are references from the geometry
		std::vector <int64_t> materials;

		// Children
		std::set <int64_t> children;

		// Construct initialized the IDs
		Object(const core::UUID &uuid_, Scene *const reference_)
			: Unique(uuid_),
			reference(reference_),
			geometry(-1) {}

		// Extracting geometry
		bool has_geometry() const {
			return geometry != -1;
		}

		const auto &get_geometry() const {
			return reference->geometries[geometry];
		}

		// Extracting materials
		bool has_materials() const {
			return materials.size();
		}

		size_t get_material_count() const {
			return materials.size();
		}
		
		const auto &get_material(size_t i) const {
			return reference->materials[i];
		}

		const auto &get_local_material(size_t i) const {
			size_t index = materials[i];
			return reference->materials[index];
		}
	};

	// Concrete objects
	std::set <int64_t> root;
	std::map <int64_t, int64_t> mapping;
	std::list <Object> objects;
	// TODO: paged list structure with addresses preserved...

	Object &add();
	Object &add_root();

	void add(const engine::ImportedAsset &);

	Object &operator[](int64_t);
	const Object &operator[](int64_t) const;

	void write(const std::filesystem::path &);
};

} // namespace jvl::core
