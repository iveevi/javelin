#pragma once

#include "../glsl.hpp"
#include "../../aliases.hpp"

namespace jvl::ire {

// TODO: wrapper intrinsic for arrays...

struct gl_MeshPerVertexEXT {
	vec4 gl_Position;
	f32 gl_PointSize;

	explicit gl_MeshPerVertexEXT() = default;

	auto layout() const {
		// TODO: needs to be special or phantom
		// TODO: specification!
		return uniform_layout("gl_MeshPerVertexEXT",
			named_field(gl_Position),
			named_field(gl_PointSize));
	}

	gl_MeshPerVertexEXT(const cache_index_t &c) {
		layout().remove_const().ref_with(c);
	}
};

using gl_MeshPerVertexEXT_t = __glsl_array <gl_MeshPerVertexEXT, thunder::glsl_intrinsic_gl_MeshVerticesEXT>;
using gl_PrimitiveTriangleIndicesEXT_t = __glsl_array <uvec3, thunder::glsl_intrinsic_gl_PrimitiveTriangleIndicesEXT>;

static const gl_MeshPerVertexEXT_t gl_MeshVerticesEXT;
static const gl_PrimitiveTriangleIndicesEXT_t gl_PrimitiveTriangleIndicesEXT;

} // namespace jvl::ire