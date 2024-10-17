#pragma once

#include "intrinsics.hpp"
#include "flags.hpp"

namespace jvl::tsg {

// Determine the shader stage based on the signature, and record any errors
template <typename ... Args>
struct shader_stage_classifier {
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eNone;

	// Only so that the default is a subroutine
	static constexpr ShaderStageFlags resolved = ShaderStageFlags::eSubroutine;

	static constexpr ClassificationErrorFlags status() {
		return ClassificationErrorFlags::eOk;
	}
};

template <typename ... Args>
struct shader_stage_classifier <VertexIntrinsics, Args...> {
	using prev = shader_stage_classifier <Args...>;
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eVertex;
	static constexpr ShaderStageFlags resolved = flags;

	static constexpr ClassificationErrorFlags status() {
		if constexpr (has(prev::flags, ShaderStageFlags::eVertex))
			return prev::status() + ClassificationErrorFlags::eDuplicateVertex;

		if constexpr (prev::flags != ShaderStageFlags::eNone)
			return prev::status() + ClassificationErrorFlags::eMultipleStages;

		return prev::status();
	}
};

template <typename ... Args>
struct shader_stage_classifier <FragmentIntrinsics, Args...> {
	using prev = shader_stage_classifier <Args...>;
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eFragment;
	static constexpr ShaderStageFlags resolved = flags;

	static constexpr ClassificationErrorFlags status() {
		if constexpr (has(prev::flags, ShaderStageFlags::eFragment))
			return prev::status() + ClassificationErrorFlags::eDuplicateFragment;

		if constexpr (prev::flags != ShaderStageFlags::eNone)
			return prev::status() + ClassificationErrorFlags::eMultipleStages;

		return prev::status();
	}
};

template <typename ... Args>
struct shader_stage_classifier <std::tuple <Args...>> : shader_stage_classifier <Args...> {};

// Reporting errors through instantiation of the type
template <ClassificationErrorFlags F>
struct verify_shader_stage_classification {
	static_assert(!has(F, ClassificationErrorFlags::eDuplicateVertex),
		"Vertexshader program must use a single stage intrinsic parameter");

	static_assert(!has(F, ClassificationErrorFlags::eDuplicateFragment),
		"Fragment shader program must use a single stage intrinsic parameter");

	static_assert(!has(F, ClassificationErrorFlags::eMultipleStages),
		"Shader program cannot use multiple stage intrinsics");
};

} // jvl::tsg