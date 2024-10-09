#pragma once

#include <cstdint>

// Available viewport modes
enum class RenderMode : int32_t {
	eAlbedo,
	eNormalSmooth,
	eNormalGeometric,
	eTextureCoordinates,
	eTriangles,
	eObject,
	eDepth,
	eBackup,
	eCount,
};

static constexpr const char *tbl_render_mode[] = {
	"Albedo",
	"Normal (smooth)",
	"Normal (geometric)",
	"Texture coordinates",
	"Triangles",
	"Object",
	"Depth",
	"Backup",
	"__end",
};

static_assert(uint32_t(RenderMode::eCount) + 1 == sizeof(tbl_render_mode)/sizeof(const char *));