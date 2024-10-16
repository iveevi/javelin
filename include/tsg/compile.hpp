#pragma once

#include "artifact.hpp"
#include "classify.hpp"
#include "exporting.hpp"
#include "initializer.hpp"
#include "layouts.hpp"
#include "lifting.hpp"
#include "signature.hpp"

namespace jvl::tsg {

// Full shader function compilation flow
template <typename F>
auto compile_function(const std::string &name, const F &ftn)
{
	// Light-weight semantic checking
	using T = decltype(function_breakdown(ftn));
	using S = signature <typename T::base>;
	using R = typename S::returns;
	using Args = typename S::arguments;

	// TODO: skip code gen if this fails
	constexpr auto status = classifier <Args> ::status();
	constexpr auto flags = classifier <Args> ::resolved;
	verify_classification <status> check;

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
	shader_args_initializer(arguments);
	auto result = std::apply(ftn, arguments);
	exporting(result);

	em.pop();

	return buffer;
}

} // namespace jvl::tsg