#include <deque>

// Glslang and SPIRV-Tools
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "thunder/c_like_generator.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage_unit.hpp"
#include "thunder/properties.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

//////////////////////////////////
// Generation: GLSL source code //
//////////////////////////////////

// Helper methods
static std::string layout_interpolation_string(QualifierKind kind)
{
	switch (kind) {
	case layout_in_flat:
	case layout_out_flat:
		return "flat ";
	case layout_in_noperspective:
	case layout_out_noperspective:
		return "noperspective ";
	case layout_in_smooth:
	case layout_out_smooth:
		return "";
	default:
		break;
	}

	return "<interp:?>";
}

static std::string image_string(QualifierKind kind)
{
	PrimitiveType result = image_result(kind);
	int32_t dimensions = image_dimension(kind);
	switch (result) {
        case ivec4:
                return fmt::format("iimage{}D", dimensions);
        case uvec4:
                return fmt::format("uimage{}D", dimensions);
        case vec4:
                return fmt::format("image{}D", dimensions);
        default:
                break;
        }

        return fmt::format("<?>image{}D", dimensions);
}

static std::string sampler_string(QualifierKind kind)
{
	PrimitiveType result = sampler_result(kind);
	int32_t dimensions = sampler_dimension(kind);
	switch (result) {
        case ivec4:
                return fmt::format("isampler{}D", dimensions);
        case uvec4:
                return fmt::format("usampler{}D", dimensions);
        case vec4:
                return fmt::format("sampler{}D", dimensions);
        default:
                break;
        }

        return fmt::format("<?>sampler{}D", dimensions);
}

// Managing generators
generator_list LinkageUnit::configure_generators() const
{
	generator_list generators;

	for (size_t i = 0; i < functions.size(); i++) {
		auto &function = functions[i];
		auto &map = types[i];

		std::map <Index, std::string> structs;
		for (auto &[k, v] : map)
			structs[k] = aggregates[v].name;

		auto aux = detail::auxiliary_block_t(function, structs);

		generators.emplace_back(aux);
	}

	return generators;
}

// Retrieve the underlying aggregate for a qualifier
auto underlying_aggregate(const std::vector <Function> &functions,
			  const std::vector <TypeMap> &types,
			  const std::vector <Aggregate> &aggregates,
			  local_layout_type llt)
{
	auto &map = types[llt.function];
	auto &function = functions[llt.function];

	auto &atom = function.atoms[llt.index];
	JVL_ASSERT(atom.is <Qualifier> (), "expected buffer atom to be a qualifier:\n{}", atom);

	auto &original = atom.as <Qualifier> ();

	JVL_ASSERT(map.contains(original.underlying),
		"aggregate structure corresponding to "
		"@{} is missing", original.underlying);

	auto &idx = map.at(original.underlying);

	return aggregates[idx];
}

// Generating the aggregates
void generate_aggregates(std::string &result,
			 const generator_list &generators,
			 const std::vector <Aggregate> &aggregates)
{
	for (auto &aggregate : aggregates) {
		if (aggregate.phantom)
			continue;

		JVL_ASSERT(aggregate.function >= 0 && aggregate.function < generators.size(),
			"aggregate function is out of bounds "
			" ({} when there are only {} generators)",
			aggregate.function, generators.size());

		auto &generator = generators[aggregate.function];

		result += "struct " + aggregate.name + " {\n";
		for (size_t i = 0; i < aggregate.fields.size(); i++) {
			auto &field = aggregate.fields[i];
			auto ts = generator.type_to_string(field);
			result += fmt::format("    {} {}{};\n", ts.pre, field.name, ts.post);
		}
		result += "};\n\n";
	}
}

// Generating the buffer references
void generate_references(std::string &result,
			 const generator_list &generators,
			 const std::vector <Function> &functions,
			 const std::vector <TypeMap> &types,
			 const std::vector <Aggregate> &aggregates,
			 const std::map <Index, local_layout_type> &references)
{
	for (auto &[binding, reference] : references) {
		auto &generator = generators[reference.function];

		std::string formats = "";
		for (auto &k : reference.extra) {
			if (k == scalar)
				formats += ", scalar";
		}

		result += fmt::format("layout (buffer_reference{}) buffer BufferReference{}\n", formats, binding);
		result += "{\n";

		auto aggregate = underlying_aggregate(functions, types, aggregates, reference);

		// Replace with fields
		for (size_t i = 0; i < aggregate.fields.size(); i++) {
			auto &field = aggregate.fields[i];
			auto ts = generator.type_to_string(field);
			result += fmt::format("    {} {}{};\n", ts.pre, field.name, ts.post);
		}

		result += "};\n\n";
	}
}

// Generating layout inputs and outputs
void generate_layout_io(std::string &result,
			const generator_list &generators,
			const std::vector <Function> &functions,
			const std::string &iot,
			const auto &list)
{
	for (auto &[b, llt] : list) {
		// TODO: a combined generator...
		auto &types = functions[llt.function].types;
		auto &generator = generators[llt.function];

		auto ts = generator.type_to_string(types[llt.index]);
		auto interpolation = layout_interpolation_string(llt.kind);

		result += fmt::format("layout (location = {}) {} {}{} _l{}{}{};\n",
			b, iot, interpolation, ts.pre, iot, b, ts.post);
	}

	if (list.size())
		result += "\n";
}

void generate_push_constant(std::string &result,
			    const generator_list &generators,
			    const std::vector <Function> &functions,
			    const std::vector <TypeMap> &types,
			    const std::vector <Aggregate> &aggregates,
			    push_constant_info pc)
{
	if (pc.index == -1)
		return;

	auto &generator = generators[pc.function];

	result += "layout (push_constant) uniform PushConstant\n";
	result += "{\n";

	auto aggregate = underlying_aggregate(functions, types, aggregates, pc);

	// Replace with fields
	for (size_t i = 0; i < aggregate.fields.size(); i++) {
		auto &field = aggregate.fields[i];
		auto ts = generator.type_to_string(field);

		if (i == 0 && pc.offset != 0)
			result += fmt::format("    layout (offset = {}) {} {}{};\n", pc.offset, ts.pre, field.name, ts.post);
		else
			result += fmt::format("    {} {}{};\n", ts.pre, field.name, ts.post);
	}

	result += "} _pc;\n\n";
}

void generate_uniforms(std::string &result,
			    const generator_list &generators,
			    const std::vector <Function> &functions,
			    const auto &list)
{
	for (auto &[b, llt] : list) {
		auto &generator = generators[llt.function];

		auto ts = generator.type_to_string(QualifiedType::concrete(llt.index));

		result += fmt::format("layout (binding = {}) uniform ublock{}\n", b, b);
		result += "{\n";
		result += fmt::format("    {} _uniform{};\n", ts.pre + ts.post, b);
		result += "};\n\n";
	}
}

void generate_buffers(std::string &result,
		     const generator_list &generators,
		     const std::vector <Function> &functions,
		     const std::vector <TypeMap> &types,
		     const std::vector <Aggregate> &aggregates,
		     const std::map <Index, local_layout_type> &buffers)
{
	for (auto &[binding, buffer] : buffers) {
		auto &generator = generators[buffer.function];

		std::string formats = "";
		std::string modifier = "";
		for (auto &k : buffer.extra) {
			if (k == writeonly)
				modifier += "writeonly ";
			if (k == readonly)
				modifier += "readonly ";
			if (k == scalar)
				formats += ", scalar";
		}

		result += fmt::format("layout (binding = {}{}) {}buffer bblock{}\n",
			binding, formats,
			modifier, binding);

		result += "{\n";

		auto aggregate = underlying_aggregate(functions, types, aggregates, buffer);

		// Replace with fields
		for (size_t i = 0; i < aggregate.fields.size(); i++) {
			auto &field = aggregate.fields[i];
			auto ts = generator.type_to_string(field);
			result += fmt::format("    {} {}{};\n", ts.pre, field.name, ts.post);
		}

		result += fmt::format("}} _buffer{};\n\n", binding);
	}
}

void generate_shared(std::string &result,
		     const generator_list &generators,
		     const std::vector <Function> &functions,
		     const auto &list)
{
	for (auto &[b, llt] : list) {
		auto &types = functions[llt.function].types;
		auto &generator = generators[llt.function];

		auto ts = generator.type_to_string(types[llt.index]);

		result += fmt::format("shared {} _shared{}{};\n", ts.pre, b, ts.post);
	}

	if (list.size())
		result += "\n";
}

std::string format_kind_string(QualifierKind kind)
{
	switch (kind) {
	case format_rgba32f:
		return "rgba32f";
	case format_rgba16f:
		return "rgba16f";
	default:
		break;
	}

	JVL_ABORT("qualifier {} is not a format", tbl_qualifier_kind[kind]);
}

void generate_images(std::string &result, const auto &images)
{
	for (const auto &[binding, llt] : images) {
		std::string modifier = " ";
		std::string inner = "";
		for (auto &k : llt.extra) {
			if (k == writeonly)
				modifier += "writeonly ";
			else if (k == readonly)
				modifier += "readonly ";
			else if (format_kind(k))
				inner = format_kind_string(k);
			else
				JVL_WARNING("unhandled image qualifier {}", tbl_qualifier_kind[k]);
		}

		if (!inner.empty())
			inner = ", " + inner;

		result += fmt::format("layout (binding = {}{}) uniform{}{} _image{};\n",
			binding,
			inner,
			modifier,
			image_string(llt.kind),
			binding);
	}

	if (images.size())
		result += "\n";
}

void generate_samplers(std::string &result, const auto &samplers)
{
	for (const auto &[binding, llt] : samplers) {
		std::string array = "";
		if (llt.size) {
			auto size = llt.size.value();
			array = size >= 0 ? fmt::format("[{}]", size) : fmt::format("[]");
		}

		result += fmt::format(
			"layout (binding = {}) uniform {} _sampler{}{};\n",
			binding,
			sampler_string(llt.kind),
			binding,
			array
		);
	}

	if (samplers.size())
		result += "\n";
}

void generate_special(std::string &result,
		      const generator_list &generators,
		      const std::vector <Function> &functions,
		      const auto &special)
{
	for (const auto &[kind, maps] : special) {
		// TODO: method
		switch (kind) {

		case task_payload:
		{
			auto &st = maps.at(-1);
			auto &types = functions[st.function].types;
			auto &generator = generators[st.function];
			auto ts = generator.type_to_string(types[st.index]);
			result += fmt::format("taskPayloadSharedEXT {} _task_payload;\n\n", ts.pre + ts.post);
		} break;

		case hit_attribute:
		{
			auto &st = maps.at(-1);
			auto &types = functions[st.function].types;
			auto &generator = generators[st.function];
			auto ts = generator.type_to_string(types[st.index]);
			result += fmt::format("hitAttributeEXT {} _hit_attribute;\n\n", ts.pre + ts.post);
		} break;

		case acceleration_structure:
		{
			for (auto &[binding, st] : maps) {
				result += fmt::format("layout (binding = {}) uniform accelerationStructureEXT _accel{};\n",
					binding, binding);
			}

			result += "\n";
		} break;

		case ray_tracing_payload:
		case ray_tracing_payload_in:
		{
			std::string type = "rayPayloadEXT";
			if (kind == ray_tracing_payload_in)
				type = "rayPayloadInEXT";

			for (auto &[binding, st] : maps) {
				auto &types = functions[st.function].types;
				auto &generator = generators[st.function];
				auto ts = generator.type_to_string(types[st.index]);
				result += fmt::format("layout (location = {}) {} {} _ray_payload{}{};\n",
					binding, type, ts.pre,
					binding, ts.post);
			}

			result += "\n";
		} break;

		default:
			JVL_WARNING("unsupported special global: {}", tbl_qualifier_kind[kind]);
			break;
		}
	}
}

// Topological sorting functions by dependencies
auto topological_sort(const std::map <Index, std::set <Index>> &dependencies)
{
	std::set <Index> included;
	std::deque <Index> sorted;

	std::deque <Index> proc;
	for (size_t i = 0; i < dependencies.size(); i++)
		proc.push_back(i);

	while (proc.size()) {
		Index i = proc.front();
		proc.pop_front();

		if (included.contains(i))
			continue;

		included.insert(i);
		sorted.push_front(i);

		JVL_ASSERT(dependencies.contains(i), "dependencies missing function index {}", i);
		for (auto j : dependencies.at(i))
			proc.push_front(j);
	}

	return sorted;
}

// Primary generation routine
std::string LinkageUnit::generate_glsl() const
{
	std::string result;

	result += "#version 460\n";
	result += "\n";

	// Include required extensions
	for (auto &ext : extensions)
		result += fmt::format("#extension {} : require\n", ext);

	if (extensions.size())
		result += "\n";

	// Create the generators
	auto generators = configure_generators();

	// Special global states
	if (local_size) {
		result += fmt::format("layout ("
			"local_size_x = {}, "
			"local_size_y = {}, "
			"local_size_z = {}) in;"
			"\n\n",
			local_size->x,
			local_size->y,
			local_size->z);
	}

	if (mesh_shader_size) {
		result += fmt::format("layout ("
			"triangles, "
			"max_vertices = {}, "
			"max_primitives = {}) out;"
			"\n\n",
			mesh_shader_size->x,
			mesh_shader_size->y);
	}

	// Declare the aggregates used
	generate_aggregates(result, generators, aggregates);

	// Declare all buffer references
	generate_references(result,
		generators,
		functions,
		types,
		aggregates,
		globals.references);

	// Globals: layout inputs and outputs
	generate_layout_io(result, generators, functions, "in", globals.inputs);
	generate_layout_io(result, generators, functions, "out", globals.outputs);

	// Globals: push constants
	generate_push_constant(result, generators, functions, types, aggregates, globals.push_constant);

	// Globals: uniform variables and buffers
	generate_uniforms(result, generators, functions, globals.uniforms);

	// TODO: make these methods to avoid passing things...
	generate_buffers(result,
		generators,
		functions,
		types,
		aggregates,
		globals.buffers);

	// Globals: shared variables
	generate_shared(result, generators, functions, globals.shared);

	// Globals: images and samplers
	generate_images(result, globals.images);
	generate_samplers(result, globals.samplers);

	// Globals: special
	generate_special(result, generators, functions, globals.special);

	// Fix dependencies to use local indices
	auto resolved_dependencies = dependencies;

	// Generate each of the functions
	auto sorted = topological_sort(resolved_dependencies);

	while (sorted.size()) {
		Index i = sorted.front();
		sorted.pop_front();

		auto &function = functions[i];
		auto &generator = generators[i];

		generate_function(result, generator, function);

		if (sorted.size())
			result += "\n";
	}

	return result;
}

} // namespace jvl::thunder
