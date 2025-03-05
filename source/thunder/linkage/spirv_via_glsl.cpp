// Glslang and SPIRV-Tools
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "thunder/linkage_unit.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

////////////////////////////////////////////////
// Generation: SPIRV compilation with glslang //
////////////////////////////////////////////////

// Compiling shaders
constexpr EShLanguage translate_shader_stage(const vk::ShaderStageFlagBits &stage)
{
	switch (stage) {
	case vk::ShaderStageFlagBits::eVertex:
		return EShLangVertex;
	case vk::ShaderStageFlagBits::eTessellationControl:
		return EShLangTessControl;
	case vk::ShaderStageFlagBits::eTessellationEvaluation:
		return EShLangTessEvaluation;
	case vk::ShaderStageFlagBits::eGeometry:
		return EShLangGeometry;
	case vk::ShaderStageFlagBits::eFragment:
		return EShLangFragment;
	case vk::ShaderStageFlagBits::eCompute:
		return EShLangCompute;
	case vk::ShaderStageFlagBits::eTaskEXT:
		return EShLangTask;
	case vk::ShaderStageFlagBits::eMeshNV:
		return EShLangMesh;
	case vk::ShaderStageFlagBits::eRaygenNV:
		return EShLangRayGenNV;
	case vk::ShaderStageFlagBits::eAnyHitNV:
		return EShLangAnyHitNV;
	case vk::ShaderStageFlagBits::eClosestHitNV:
		return EShLangClosestHitNV;
	case vk::ShaderStageFlagBits::eMissNV:
		return EShLangMissNV;
	case vk::ShaderStageFlagBits::eIntersectionNV:
		return EShLangIntersectNV;
	case vk::ShaderStageFlagBits::eCallableNV:
		return EShLangCallableNV;
	default:
		break;
	}

	return EShLangVertex;
}

std::vector <uint32_t> LinkageUnit::generate_spirv_via_glsl(const vk::ShaderStageFlagBits &flags) const
{
	EShLanguage stage = translate_shader_stage(flags);

	std::string glsl = generate_glsl();

	const char *shaderStrings[] { glsl.c_str() };

	glslang::SpvOptions options;
	options.generateDebugInfo = true;

	glslang::TShader shader(stage);

	shader.setStrings(shaderStrings, 1);
	shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv,
			    glslang::EShTargetLanguageVersion::EShTargetSpv_1_6);

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages) (EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

	if (!shader.parse(GetDefaultResources(), 460, false, messages)) {
		std::string log = shader.getInfoLog();
		JVL_ERROR("failed to compile to SPIRV:\n{}", log);
		fmt::println("{}", glsl);
		return {};
	}

	// Link the program
	glslang::TProgram program;

	program.addShader(&shader);

	if (!program.link(messages)) {
		std::string log = program.getInfoLog();
		JVL_ERROR("failed to link SPIRV code:\n{}", log);
		fmt::println("{}", glsl);
		return {};
	}

	std::vector <uint32_t> spirv;
	{
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &options);
	}
	return spirv;
}

} // namespace jvl::thunder
