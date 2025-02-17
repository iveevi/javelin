#pragma once

#include "../glsl.hpp"
#include "../../aliases.hpp"

namespace jvl::ire {

struct __gl_MeshPerVertexEXT {
	vec4 gl_Position;
	f32 gl_PointSize;

	explicit __gl_MeshPerVertexEXT() = default;

	auto layout() {
		// TODO: needs to be special or phantom
		// TODO: specification!
		return layout_from("gl_MeshPerVertexEXT",
			verbatim_field(gl_Position),
			verbatim_field(gl_PointSize));
	}

	__gl_MeshPerVertexEXT(const cache_index_t &idx) {
		layout().link(idx.id);
	}
};

using gl_MeshPerVertexEXT_t = __glsl_array <__gl_MeshPerVertexEXT, thunder::glsl_intrinsic_gl_MeshVerticesEXT>;
using gl_PrimitiveTriangleIndicesEXT_t = __glsl_array <uvec3, thunder::glsl_intrinsic_gl_PrimitiveTriangleIndicesEXT>;

static const gl_MeshPerVertexEXT_t gl_MeshVerticesEXT;
static const gl_PrimitiveTriangleIndicesEXT_t gl_PrimitiveTriangleIndicesEXT;

} // namespace jvl::ire