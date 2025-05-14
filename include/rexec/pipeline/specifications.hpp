#pragma once

#include <vulkan/vulkan.hpp>

namespace jvl::rexec {

	enum PipelineKind {
	eUnknown,
	eTraditional,
	eMeshShader,
	eRayTracing,
};

// TODO: use resource index concept...
template <typename VertexElement,	// Vertex buffer element
	  typename VertexConstants,	// Push constants for vertex shader
	  typename FragmentConstants,	// Push constants for fragment shader
	  typename Bindables>		// Other resources linked through descriptor sets
struct TraditionalPipeline {
	static constexpr PipelineKind kind = eTraditional;

	vk::Pipeline handle;
	vk::PipelineLayout layout;

	// TODO: enable only if bindables exist
	vk::DescriptorSetLayout dsl;
};

template <typename Bindables>
struct MeshShaderPipeline {
	static constexpr PipelineKind kind = eMeshShader;
};

template <typename Bindables>
struct RayTracingPipeline {
	static constexpr PipelineKind kind = eRayTracing;
};

} // namespace jvl::rexec