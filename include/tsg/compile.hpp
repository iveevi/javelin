#pragma once

#include "artifact.hpp"
#include "classify.hpp"
#include "exporting.hpp"
#include "initializer.hpp"
#include "layouts.hpp"
#include "lifting.hpp"
#include "signature.hpp"
#include "error.hpp"

namespace jvl::tsg {

// One-to-one argument checker
template <typename T, typename U>
struct compare_one_to_one {
	static constexpr auto value = shader_compiler_error(eOk);
};

template <generic T, generic U>
struct compare_one_to_one <PushConstant <T>, PushConstant <U>> {
	static constexpr auto value = shader_compiler_error(eMultiplePushConstants);
};

// TODO: how to ensure symmetry

template <typename T, typename ... Args>
struct compare_vector {
	static constexpr size_t N = sizeof...(Args);

	static constexpr std::array <shader_compiler_error, N> value {
		compare_one_to_one <T, Args> ::value...
	};
};

// "Kernel" for checking arguments
template <typename T>
struct verify_argument_structure {
	static constexpr shader_compiler_error value = shader_compiler_error(eUnknown);
};

template <typename ... Args>
struct verify_argument_structure <std::tuple <Args...>> {
	// Returns the first error encountered
	static constexpr shader_compiler_error assess() {
		constexpr size_t N = sizeof...(Args);

		// Generate the error matrix
		std::array <std::array <shader_compiler_error, N>, N> status {
			compare_vector <Args, Args...> ::value...
		};
		
		// Get the first problem if any
		for (size_t i = 0; i < N; i++) {
			for (size_t j = 0; j < N; j++) {
				if (i != j && status[i][j].type != eOk)
					return status[i][j];
			}
		}

		// TODO: return this array and check everything in the compile function?

		return shader_compiler_error(eOk);
	}

	static constexpr auto value = assess();
};

// Full shader function compilation flow
template <typename F>
auto compile_function(const std::string &name, const F &ftn)
{
	// Light-weight semantic checking
	using T = decltype(function_breakdown(ftn));
	using S = signature <typename T::base>;
	using R = typename S::returns;
	using Args = typename S::arguments;

	// Check compatibility of shader stage intrinsics provided
	constexpr auto status = shader_stage_classifier <Args> ::status();
	constexpr auto flags = shader_stage_classifier <Args> ::resolved;
	verify_shader_stage_classification <status> check;

	// Check for duplicate arguments types (e.g. push constants, bindings, etc.)
	constexpr auto value = verify_argument_structure <Args> ::value;
	assess_shader_compiler_error <value> ();

	// Secondary pass given the shader stage
	using lifted_results = lift_result <R> ::type;
	using lifted_args = lift_argument <Args> ::type;
	using dependencies = collect_layout_inputs <flags, lifted_args> ::type;
	using results = collect_layout_outputs<flags, lifted_results> ::type;

	// Begin code generation stage
	auto &em = ire::Emitter::active;
	
	auto buffer = compiled_artifact_assembler <flags, dependencies, results> ::make();
	buffer.name = name;

	em.push(buffer);

	lifted_args arguments;
	shader_args_initializer(Args(), arguments);
	auto result = std::apply(ftn, arguments);
	exporting(result);

	em.pop();

	return buffer;
}

} // namespace jvl::tsg