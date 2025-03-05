#include <fstream>

#include <common/logging.hpp>

#include "ngf.hpp"

MODULE(ngf-load);

ImportedNGF ImportedNGF::load(const std::filesystem::path &path)
{
	ImportedNGF ngf;

	std::ifstream fin(path);
	JVL_ASSERT(fin.good(), "Bad ngf file {}", path.c_str());

	int32_t sizes[3];
	fin.read(reinterpret_cast <char *> (sizes), sizeof(sizes));

	JVL_INFO("patches={}", sizes[0]);
	JVL_INFO("vertices={}", sizes[1]);
	JVL_INFO("features={}", sizes[2]);

	std::vector <glm::ivec4> patches;
	std::vector <glm::vec3> vertices;
	std::vector <float> features;

	patches.resize(sizes[0]);
	vertices.resize(sizes[1]);
	features.resize(sizes[1] * sizes[2]);

	ngf.patch_count = sizes[0];
	ngf.feature_size = sizes[2];

	fin.read(reinterpret_cast <char *> (vertices.data()), vertices.size() * sizeof(glm::vec3));
	fin.read(reinterpret_cast <char *> (features.data()), features.size() * sizeof(float));
	fin.read(reinterpret_cast <char *> (patches.data()), patches.size() * sizeof(glm::ivec4));

	std::array <Tensor, LAYERS> weights;
	for (int32_t i = 0; i < LAYERS; i++) {
		int32_t sizes[2];
		fin.read(reinterpret_cast <char *> (sizes), sizeof(sizes));
		JVL_INFO("weight matrix with size {}x{}", sizes[0], sizes[1]);

		Tensor w;
		w.width = sizes[0];
		w.height = sizes[1];
		w.resize(sizes[0] * sizes[1]);
		fin.read(reinterpret_cast <char *> (w.data()), w.size() * sizeof(float));

		weights[i] = w;
	}

	std::array <Tensor, LAYERS> biases;
	for (int32_t i = 0; i < LAYERS; i++) {
		int32_t size;
		fin.read(reinterpret_cast <char *> (&size), sizeof(size));
		JVL_INFO("bias vector with size {}", size);

		Tensor w;
		w.width = size;
		w.height = 1;
		w.resize(size);
		fin.read(reinterpret_cast <char *> (w.data()), w.size() * sizeof(float));

		biases[i] = w;
	}

	ngf.patches = patches;
	ngf.features = features;
	ngf.weights = weights;
	ngf.biases = biases;

	// TODO: put this elsewhere..
	// Need special care for vertices to align them properly
	ngf.vertices.resize(vertices.size());
	for (int32_t i = 0; i < vertices.size(); i++)
		ngf.vertices[i] = glm::vec4(vertices[i], 0.0f);

	return ngf;
}

HomogenizedNGF HomogenizedNGF::from(const ImportedNGF &ngf)
{
	// Concatenate the biases into a single buffer
	std::vector <float> biases;
	for (int32_t i = 0; i < LAYERS; i++)
		biases.insert(biases.end(), ngf.biases[i].begin(), ngf.biases[i].end());

	// Align to vec4 size
	size_t fixed = (biases.size() + 3)/4;
	biases.resize(4 * fixed);

	// Weights (transposed)
	size_t w0c = ngf.weights[0].height;
	std::vector <float> W0(64 * w0c);
	std::vector <float> W1(64 * 64);
	std::vector <float> W2(64 * 64);
	std::vector <float> W3 = ngf.weights[3];

	for (size_t i = 0; i < 64; i++) {
		for (size_t j = 0; j < 64; j++) {
			W1[j * 64 + i] = ngf.weights[1][i * 64 + j];
			W2[j * 64 + i] = ngf.weights[2][i * 64 + j];
		}

		for (size_t j = 0; j < w0c; j++)
			W0[j * 64 + i] = ngf.weights[0][i * w0c + j];
	}

	// Feature vector
	std::vector <glm::vec4> features(ngf.patch_count * ngf.feature_size);

	for (size_t i = 0; i < ngf.patch_count; i++) {
		glm::ivec4 complex = ngf.patches[i];
		for (size_t j = 0; j < ngf.feature_size; j++) {
			float f0 = ngf.features[complex.x * ngf.feature_size + j];
			float f1 = ngf.features[complex.y * ngf.feature_size + j];
			float f2 = ngf.features[complex.z * ngf.feature_size + j];
			float f3 = ngf.features[complex.w * ngf.feature_size + j];
			features[i * ngf.feature_size + j] = glm::vec4(f0, f1, f2, f3);
		}
	}

	return HomogenizedNGF {
		.patches = ngf.patches,
		.vertices = ngf.vertices,
		.features = features,
		.biases = biases,
		.W0 = W0, .W1 = W1,
		.W2 = W2, .W3 = W3,
	};
}