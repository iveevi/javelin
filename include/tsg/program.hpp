#pragma once

#include "artifact.hpp"
#include "error.hpp"
#include "flags.hpp"
#include "requirement.hpp"
#include "resolution.hpp"

namespace jvl::tsg {

// Full shader program construction
template <ShaderStageFlags flags = ShaderStageFlags::eNone, typename ... Args>
struct Program {};

// From zero
template <>
struct Program <ShaderStageFlags::eNone> {
	template <ShaderStageFlags added, specifier ... Specifiers>
	friend auto operator<<(const Program &program, const CompiledArtifact <added, Specifiers...> &compiled) {
		return Program <added, Specifiers...> {
			static_cast <thunder::TrackedBuffer> (compiled)
		};
	}
};

// Vertex and fragment sub-programs
template <specifier ... Specifiers>
struct Program <ShaderStageFlags::eVertex, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;

	template <specifier ... AddedSpecifiers>
	friend auto operator<<(const Program &program, const CompiledArtifact <ShaderStageFlags::eFragment, AddedSpecifiers...> &compiled) {
		using result_program = Program <ShaderStageFlags::eVxF, Specifiers..., AddedSpecifiers...>;
		static constexpr auto value = verify_requirements_resolution <typename result_program::satisfied> ::value;
		assess_shader_compiler_error <value> ();
		return Program <ShaderStageFlags::eVxF, Specifiers..., AddedSpecifiers...> {
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
		using result_program = Program <ShaderStageFlags::eVxF, Specifiers..., AddedSpecifiers...>;
		static constexpr auto value = verify_requirements_resolution <typename result_program::satisfied> ::value;
		assess_shader_compiler_error <value> ();
		return result_program {
			static_cast <thunder::TrackedBuffer> (compiled),
			program.ir_fragment,
		};
	}
};

// Complete vertex -> fragment shader program
template <specifier ... Specifiers>
struct Program <ShaderStageFlags::eVxF, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;
	thunder::TrackedBuffer ir_fragment;
		
	// Layout compatibility verification
	using raw_requirements = RequirementVector <typename requirement_for <Specifiers> ::type...>;
	using requirements = typename raw_requirements::filtered;
	using satisfied = typename resolve_requirement_vector <requirements, Specifiers...> ::result;

	// TODO: extract descriptor set layout
};

} // namespace jvl::tsg