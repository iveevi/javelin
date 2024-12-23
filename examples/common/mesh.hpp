#pragma once

#include "types.hpp"

namespace jvl::core {

struct Mesh {
	// Vertex property keys
	static constexpr char position_key[] = "position";
	static constexpr char normal_key[] = "normal";
	static constexpr char uv_key[] = "uv";

	// Face property keys
	static constexpr char triangle_key[] = "triangles";
	static constexpr char quadrilateral_key[] = "quadrilaterals";
	static constexpr char material_key[] = "material";

	size_t vertex_count;
	property <typed_vector> vertex_properties;
	property <typed_vector> face_properties;
};

} // namespace jvl::core
