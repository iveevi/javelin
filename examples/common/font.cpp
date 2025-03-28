#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_OUTLINE_H

#include <common/logging.hpp>

#include "font.hpp"

MODULE(font);

struct OutlineDecomposeData {
	std::vector <FontCurve> &curves;
	glm::vec2 position;
};

glm::vec2 glmify(const FT_Vector *to)
{
	return glm::vec2(to->x, to->y);
}

static int move_callback(const FT_Vector *to, void *user)
{
	auto *data = (OutlineDecomposeData *) user;
	data->position = glmify(to);
	return 0;
}

static int line_callback(const FT_Vector *to, void *user)
{
	auto *data = (OutlineDecomposeData*) user;
	const auto end = glmify(to);
	const auto control = (data->position + end) * 0.5f;
	data->curves.emplace_back(data->position, control, end);
	data->position = end;
	return 0;
}

static int conic_callback(const FT_Vector *control, const FT_Vector *to, void *user)
{
	auto* data = (OutlineDecomposeData *) user;
	const auto end = glmify(to);
	data->curves.emplace_back(data->position, glmify(control), end);
	data->position = end;
	return 0;
}

static int cubic_callback(const FT_Vector *, const FT_Vector *, const FT_Vector *, void *)
{
	JVL_ERROR("cubic strokes are unsupported");
	return 0;
}

Font Font::from(const std::filesystem::path &path)
{
	Font result;

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		JVL_ERROR("failed to initialize FreeType");
		return result;
	}

	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face)) {
		FT_Done_FreeType(ft);
		JVL_ERROR("failed to load font: {}", path.string());
		return result;
	}

	// Store font-wide metrics
	result.metrics = {
		(float) face->ascender,
		(float) face->descender,
		(float) face->height - (face->ascender - face->descender),
		(float) face->units_per_EM,
	};

	// Configure outline decomposition
	FT_Outline_Funcs callbacks = {
		.move_to = move_callback,
		.line_to = line_callback,
		.conic_to = conic_callback,
		.cubic_to = cubic_callback,
		.shift = 0,
		.delta = 0
	};

	// Iterate through all glyphs in font
	FT_ULong charcode = FT_Get_First_Char(face, nullptr);
	while (charcode != 0) {
		FT_UInt gindex = FT_Get_Char_Index(face, charcode);

		if (FT_Load_Glyph(face, gindex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING)) {
			charcode = FT_Get_Next_Char(face, charcode, nullptr);
			continue;
		}

		GlyphInfo info;

		// Store glyph metrics
		FT_Outline* outline = &face->glyph->outline;
		FT_BBox bbox;
		if (FT_Outline_Get_BBox(outline, &bbox)) {
			charcode = FT_Get_Next_Char(face, charcode, &gindex);
			continue;
		}

		auto min = glm::vec2(bbox.xMin, bbox.yMin);

		FT_Glyph_Metrics metrics = face->glyph->metrics;

		info.size = glm::vec2(metrics.width, metrics.height);
		info.advance = glm::vec2(metrics.horiAdvance, metrics.vertAdvance);
		info.bearing = glm::vec2(metrics.horiBearingX, metrics.horiBearingY);

		// Decompose outline if present
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
			OutlineDecomposeData data { info.curves, {0, 0} };

			FT_Outline_Decompose(&face->glyph->outline, &callbacks, &data);

			for (auto &c : info.curves) {
				c.p0 = (c.p0 - min) / info.size;
				c.p1 = (c.p1 - min) / info.size;
				c.p2 = (c.p2 - min) / info.size;
			}
		}

		result.glyphs[charcode] = info;
		charcode = FT_Get_Next_Char(face, charcode, nullptr);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	return result;
}

// TODO: font method...
std::vector <VulkanGlyph> Font::layout(const std::string &text,
				       const glm::vec2 &screen,
				       const glm::vec2 &anchor,
				       float size)
{
	std::vector <VulkanGlyph> result;

	glm::vec2 scale = size / (metrics.units_per_EM * screen);

	float line_height = scale.y * (metrics.ascender - metrics.descender + metrics.line_gap);

	glm::vec2 p = anchor / screen;

	// Anchor is upper left
	p.y -= line_height;

	for (char c : text) {
		if (c == '\n') {
			p.x = anchor.x / screen.x;
			p.y -= line_height;
			continue;
		}

		if (!glyphs.count(c))
			continue;

		const auto &info = glyphs.at(c);

		auto glyph_min = p + glm::vec2(info.bearing.x, info.bearing.y - info.size.y) * scale;
		auto glyph_max = glyph_min + info.size * scale;

		if (info.curves.size()) {
			result.push_back(VulkanGlyph {
				.bounds = { glyph_min, glyph_max },
				.address = addresses.at(c)
			});
		}

		p.x += info.advance.x * scale.x;
	}

	return result;
}

void Font::upload(VulkanResources &resources)
{
	// TODO: memory block arena... otherwise too slow to allocate and deallocate...
	auto &device = resources.device;

	auto map_glyph = [&](const GlyphInfo &g, const std::string &name) {
		auto size = sizeof(size_t) + sizeof(FontCurve) * g.curves.size();

		std::vector <uint8_t> buffer(size, 0);

		uint32_t length = g.curves.size();
		std::memcpy(buffer.data(), &length, sizeof(uint32_t));
		std::memcpy(buffer.data() + sizeof(size_t), g.curves.data(), length * sizeof(FontCurve));

		littlevk::Buffer vk_curves = resources.allocator()
			.buffer(buffer,
				vk::BufferUsageFlagBits::eStorageBuffer
				| vk::BufferUsageFlagBits::eShaderDeviceAddress);

		return device.getBufferAddress(vk_curves.buffer);
	};

	for (auto &[k, v] : glyphs)
		addresses[k] = map_glyph(v, std::string("") + char(k));

	JVL_INFO("uploaded {} glyphs to device...", glyphs.size());
}
