#pragma once

#include <vulkan/vulkan.hpp>

#include "../class.hpp"
#include "../context/fragment.hpp"
#include "../context/vertex.hpp"
#include "../error.hpp"
#include "compilation_status.hpp"
#include "compilation_traditional_verification.hpp"

namespace jvl::rexec {

template <PipelineCompilationStatus status>
constexpr void diagnose_status_traditional()
{
	if constexpr (has(status, PipelineCompilationStatus::eIncompatibleLayout)) {
		static_assert(false,
			tag(incompatible layout)
			"incompatible layout detected (i.e. T != U) between layout "
			"outputs of vertex shader and layout inputs of fragment shader");
	} else if constexpr (has(status, PipelineCompilationStatus::eMissingLayout)) {
		static_assert(false,
			tag(missing layout)
			"detected layout input specified in fragment shader which "
			"has no corresponding layout output in the vertex shader");
	} else if constexpr (has(status, PipelineCompilationStatus::eOverlappingPushConstants)) {
		static_assert(false,
			tag(overlapping push constants)
			"detected overlapping push constant ranges between the "
			"vertex shader and fragmetn shader");
	} else {
		static_assert(uint8_t(status) == 0,
			tag(traditional pipeline compile error)
			"an error was encountered when attempting to "
			"compile a traditional pipeline, but no diagnosis "
			"is currently available");
	}
}

template <ire::entrypoint_class VertexProgram,
	  ire::entrypoint_class FragmentProgram>
requires vertex_class <typename VertexProgram::rexec>
	&& fragment_class <typename FragmentProgram::rexec>
auto compile_traditional(const vk::Device &device,
			 const vk::RenderPass &renderpass,
			 const VertexProgram &vshader,
			 const FragmentProgram &fshader)
{
	using Vertex = VertexProgram::rexec;
	using Fragment = FragmentProgram::rexec;

	constexpr auto result = verify_traditional <Vertex, Fragment> ();

	if constexpr (result.status == PipelineCompilationStatus::eSuccess) {
		// Now we have all the information to compile the pipeline
		// TODO: retrieve rasterization, blending, multisampling,
		// etc. options from vertex and fragment rexecs
		return compile_traditional_vulkan(device,
			renderpass, result, vshader, fshader);
	} else {
		diagnose_status_traditional <result.status> ();
		return int();
	}
}

} // namespace jvl::rexec