#pragma once

#include "../class.hpp"
#include "../context/fragment.hpp"
#include "../context/vertex.hpp"
#include "../error.hpp"
#include "../pipeline/specifications.hpp"
#include "entrypoints_traditional.hpp"
#include "entrypoints_status.hpp"

namespace jvl::rexec {

//////////////////////////////////
// Static pipeline verification //
//////////////////////////////////

template <ire::entrypoint_class Current, ire::entrypoint_class ... Rest>
constexpr PipelineKind categorize()
{
	using rexec = typename Current::rexec;
	if constexpr (vertex_class <rexec>)
		return eTraditional;
	// if constexpr (mesh_shader_class <rexec>)
	// 	return eMeshShader;
	// if constexpr (ray_generation_class <rexec>)
	// 	return eRayTracing;
	if constexpr (sizeof...(Rest) > 0)
		return categorize <Rest...> ();
	
	return eUnknown;
}

template <ire::entrypoint_class ... Entrypoints>
requires (sizeof...(Entrypoints) > 0)
auto compile(const vk::Device &device,
	     const vk::RenderPass &renderpass,
	     const Entrypoints &... entrypoints)
{
	// Delegates to pipeline-specific compilers depending
	// on the search for a stage specific entrypoint...

	// Traditional pipeline
	// 	-> identify by vertex shader
	// Mesh shader pipeline
	// 	-> identify by mesh shader
	// Ray tracing pipeline
	// 	-> identify by ray generation shader
	constexpr auto kind = categorize <Entrypoints...> ();

	if constexpr (kind == eUnknown) {
		static_assert(false,
			tag(categorization error)
			"failed to categorize pipeline kind from given entrypoints, "
			"expecting the following leaders for each pipeline kind:\n"
			"\ttraditional -> vertex\n"
			"\tmesh shader -> mesh shader\n"
			"\tray tracing -> ray generation\n");
	} else {
		if constexpr (kind == eTraditional) {
			constexpr auto result = verify_entrypoints_traditional <0, false, 0, false, 0, Entrypoints...> ();

			if constexpr (result.status == PipelineEntrypointVerificationStatus::eSuccess) {
				// Hacky way to reorganize the entrypoint
				// while only working with their references
				auto tuple = std::tuple(std::cref(entrypoints)...);
				auto &vshader = std::get <result.vidx> (tuple).get();
				auto &fshader = std::get <result.fidx> (tuple).get();
				return compile_traditional(device, renderpass, vshader, fshader);
			} else if constexpr (result.status == PipelineEntrypointVerificationStatus::eDuplicateEntrypoint) {
				// TODO: macros for each message?
				static_assert(false,
					tag(duplicate entrypoint)
					"encountered duplicate entrypoint stages "
					"when compiling for traditional pipeline");
			} else {
				static_assert(false,
					tag(verification failure)
					"failed to verify traditional pipeline "
					"entrypoint types: unknown error");
			}
		} else {
			static_assert(false,
				tag(unsupported pipeline)
				"pipeline kind is currently unsupported");
		}
	}
}

} // namespace jvl::rexec