#pragma once

#include <map>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>

#include "vulkan_resources.hpp"

struct FontCurve {
	glm::vec2 p0;
	glm::vec2 p1;
	glm::vec2 p2;
};

struct alignas(16) VulkanGlyph {
	glm::vec4 bounds;
	uint64_t address;
};

struct GlyphInfo {
	std::vector <FontCurve> curves;
	glm::vec2 size;
	glm::vec2 advance;
	glm::vec2 bearing;
};

struct FontMetrics {
	float ascender;
	float descender;
	float line_gap;
	float units_per_EM;
};

struct Font {
	FontMetrics metrics;
	std::map <uint32_t, GlyphInfo> glyphs;
	std::map <uint32_t, uint64_t> addresses;

	std::vector <VulkanGlyph> layout(const std::string &, const glm::vec2 &, const glm::vec2 &, float);

	void upload(VulkanResources &);

	static Font from(const std::filesystem::path &);
};
