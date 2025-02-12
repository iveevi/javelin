#pragma once

#include <vector>
#include <filesystem>

#include <glm/glm.hpp>

constexpr int32_t LAYERS = 4;

struct Tensor : std::vector <float> {
	int32_t width;
	int32_t height;
};

struct ImportedNGF {
	std::vector <glm::ivec4> patches;
	std::vector <glm::vec4> vertices;
	std::vector <float> features;

	uint32_t patch_count;
	uint32_t feature_size;

	std::array <Tensor, LAYERS> weights;
	std::array <Tensor, LAYERS> biases;

	static ImportedNGF load(const std::filesystem::path &);
};

struct HomogenizedNGF {
	std::vector <glm::ivec4> patches;
	std::vector <glm::vec4> vertices;
	std::vector <glm::vec4> features;
	std::vector <float> biases;
	std::vector <float> W0;
	std::vector <float> W1;
	std::vector <float> W2;
	std::vector <float> W3;

	static HomogenizedNGF from(const ImportedNGF &);
};