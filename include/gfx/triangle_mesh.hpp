#pragma once

#include "../core/mesh.hpp"

namespace jvl::gfx {

struct TriangleMesh {
	buffer <float3> positions;
	buffer <int3> triangles;

	// TODO: additional vertex properties properties

	static std::optional <TriangleMesh> from(const core::Mesh &m) {
		TriangleMesh tm;

		if (auto opt_pos = m.vertex_properties
				.maybe_at(core::Mesh::position_key)
				.as <buffer <float3>> ())
			tm.positions = opt_pos.value();
		else
			return std::nullopt;

		if (auto opt_tris = m.face_properties
				.maybe_at(core::Mesh::triangle_key)
				.as <buffer <int3>> ())
			tm.triangles = opt_tris.value();

		// TODO: quadrilaterals -- append

		if (tm.triangles.empty())
			return std::nullopt;

		return tm;
	}
};

}
