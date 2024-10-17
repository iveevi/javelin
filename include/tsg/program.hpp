#pragma once

#include "artifact.hpp"
#include "error.hpp"
#include "flags.hpp"
#include "verification.hpp"

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
	thunder::TrackedBuffer ir_vertex;
	thunder::TrackedBuffer ir_fragment;

	template <template <typename...> typename Filter>
	using filter = Filter <Specifiers...>;
};

} // namespace jvl::tsg