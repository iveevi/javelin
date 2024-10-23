#pragma once

#include "artifact.hpp"
#include "error.hpp"
#include "flags.hpp"
#include "verification.hpp"

namespace jvl::tsg {

// Full shader program construction
template <ShaderStageFlags flags = ShaderStageFlags::eNone, specifier ... Args>
struct Program {};

// From zero
template <>
struct Program <ShaderStageFlags::eNone> {
	template <ShaderStageFlags added, specifier ... Specifiers>
	friend auto operator<<(const Program &, const CompiledArtifact <added, Specifiers...> &compiled) {
		return Program <added, Specifiers...> {
			static_cast <thunder::TrackedBuffer> (compiled)
		};
	}
};

// Vertex and fragment sub-programs
template <specifier ... Specifiers>
struct Program <ShaderStageFlags::eVertex, Specifiers...> {
	// TODO: compile to SPIRV from here right away
	thunder::TrackedBuffer ir_vertex;

	template <specifier ... AddedSpecifiers>
	friend auto operator<<(const Program &program, const CompiledArtifact <ShaderStageFlags::eFragment, AddedSpecifiers...> &compiled) {
		using specifier_pack = std::tuple <Specifiers..., AddedSpecifiers...>;
		static constexpr auto value = verify_specifiers <specifier_pack> ::value;
		diagnose_shader_compiler_error <value> ();
		return Program <ShaderStageFlags::eGraphicsVertexFragment, Specifiers..., AddedSpecifiers...> {
			program.ir_vertex,
			static_cast <thunder::TrackedBuffer> (compiled)
		};
	}
};

template <specifier ... Specifiers>
struct Program <ShaderStageFlags::eFragment, Specifiers...> {
	thunder::TrackedBuffer ir_fragment;

	template <specifier ... AddedSpecifiers>
	friend auto operator<<(const Program &program, const CompiledArtifact <ShaderStageFlags::eVertex, AddedSpecifiers...> &compiled) {
		using specifier_pack = std::tuple <Specifiers..., AddedSpecifiers...>;
		static constexpr auto value = verify_specifiers <specifier_pack> ::value;
		diagnose_shader_compiler_error <value> ();
		return Program <ShaderStageFlags::eGraphicsVertexFragment, Specifiers..., AddedSpecifiers...> {
			static_cast <thunder::TrackedBuffer> (compiled),
			program.ir_fragment,
		};
	}
};

// Complete vertex -> fragment shader program
template <specifier ... Specifiers>
struct Program <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> {
	// At this point we can compile to SPIRV
	std::vector <uint32_t> spv_vertex;
	std::vector <uint32_t> spv_fragment;

	Program(const thunder::TrackedBuffer &ir_vertex,
		const thunder::TrackedBuffer &ir_fragment) {
		// Compile the vertex program
		auto vunit = thunder::LinkageUnit();
			vunit.add(ir_vertex);

		spv_vertex = vunit.generate_spirv(vk::ShaderStageFlagBits::eVertex);

		// Compile the fragment program
		auto funit = thunder::LinkageUnit();
			funit.add(ir_fragment);

		spv_fragment = funit.generate_spirv(vk::ShaderStageFlagBits::eFragment);
	}

	// TODO: caching of the shader modules?
	auto shaders(const vk::Device &device) const {
		std::array <vk::PipelineShaderStageCreateInfo, 2> shader_stages;
		
		// Vertex shader stage
		auto vertex_module_info = vk::ShaderModuleCreateInfo()
			.setCode(spv_vertex);

		auto vertex_module = device.createShaderModule(vertex_module_info);

		shader_stages[0] = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertex_module)
			.setPName("main");

		// Fragment shader stage	
		auto fragment_module_info = vk::ShaderModuleCreateInfo()
			.setCode(spv_fragment);

		auto fragment_module = device.createShaderModule(fragment_module_info);

		shader_stages[1] = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragment_module)
			.setPName("main");

		return shader_stages;
	}
};

} // namespace jvl::tsg