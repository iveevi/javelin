#pragma once

#include <filesystem>
#include <vector>

#include "mesh.hpp"
#include "material.hpp"
#include "preset.hpp"

namespace jvl::core {

struct Scene {
	std::vector <Mesh> meshes;
	std::vector <Material> materials;

	void add(const Preset &);

	void write(const std::filesystem::path &);
};

} // namespace jvl::core
