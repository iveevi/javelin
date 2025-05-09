#pragma once

#include "../aliases.hpp"
#include "../intrinsics/glsl.hpp"

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

using gl_LaunchIDEXT_t = __glsl_uvec3 <thunder::glsl_LaunchIDEXT>;
using gl_LaunchSizeEXT_t = __glsl_uvec3 <thunder::glsl_LaunchSizeEXT>;
using gl_MeshPerVertexEXT_t = __glsl_array <__gl_MeshPerVertexEXT, thunder::glsl_MeshVerticesEXT>;
using gl_PrimitiveTriangleIndicesEXT_t = __glsl_array <uvec3, thunder::glsl_PrimitiveTriangleIndicesEXT>;

static const gl_LaunchIDEXT_t gl_LaunchIDEXT;
static const gl_LaunchSizeEXT_t gl_LaunchSizeEXT;
static const gl_MeshPerVertexEXT_t gl_MeshVerticesEXT;
static const gl_PrimitiveTriangleIndicesEXT_t gl_PrimitiveTriangleIndicesEXT;

struct mesh_shader_size {
	// TODO: primitive enum
	mesh_shader_size(uint32_t max_vertices = 1, uint32_t max_primitives = 1) {
		ire::void_platform_intrinsic_from_args(thunder::layout_mesh_shader_sizes,
			u32(max_vertices), u32(max_primitives));
	}
};

} // namespace jvl::ire