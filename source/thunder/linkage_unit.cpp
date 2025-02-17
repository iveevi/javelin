#include <deque>

// Glslang and SPIRV-Tools
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

// Native JIT libraries
#include <libgccjit.h>

#include "thunder/c_like_generator.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/gcc_jit_generator.hpp"
#include "thunder/linkage_unit.hpp"
#include "thunder/properties.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

// Equality comparison for aggregates
bool Aggregate::operator==(const Aggregate &other) const
{
	if (fields.size() != other.fields.size())
		return false;

	for (size_t i = 0; i < fields.size(); i++) {
		if (fields[i] != other.fields[i])
			return false;
	}

	return true;
}

// Functions
Function::Function(const Buffer &buffer, const std::string &name_, size_t cid_)
		: Buffer(buffer), cid(cid_), name(name_) {}

// Linkage unit methods
Index LinkageUnit::new_aggregate(size_t ftn, const std::string &name, const std::vector <Field> &fields)
{
	size_t index = aggregates.size();

	Aggregate aggr {
		.function = ftn,
		.name = name,
		.fields = fields
	};

	auto it = std::find(aggregates.begin(), aggregates.end(), aggr);
	if (it == aggregates.end()) {
		aggregates.push_back(aggr);
		return index;
	}

	return std::distance(aggregates.begin(), it);
}

// fidx; function index
// bidx; (atom) buffer index
void LinkageUnit::process_function_qualifier(Function &function, size_t fidx, Index bidx, const Qualifier &qualifier)
{
	if (image_kind(qualifier.kind)) {
		size_t binding = qualifier.numerical;
		globals.images[binding] = local_layout_type(fidx, -1, qualifier.kind);
		return;
	}

	if (sampler_kind(qualifier.kind)) {
		size_t binding = qualifier.numerical;
		globals.samplers[binding] = local_layout_type(fidx, -1, qualifier.kind);
		return;
	}

	// Qualifier behaviour; the final type,
	// offset, etc. for a qualified type is
	// that which is specified by the last
	// instruction referring to it
	switch (qualifier.kind) {

	case parameter:
	{
		size_t binding = qualifier.numerical;
		size_t size = std::max(function.args.size(), binding + 1);
		function.args.resize(size);
		function.args[binding] = function.types[bidx];
	} break;

	case push_constant:
	{
		push_constant_info info;
		info.index = bidx;
		info.function = fidx;
		info.kind = thunder::push_constant;
		info.offset = qualifier.numerical;
		info.underlying = qualifier.underlying;

		globals.push_constant = info;
	} break;

	case layout_in_flat:
	case layout_in_noperspective:
	case layout_in_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lin(fidx, qualifier.underlying, qualifier.kind);
		globals.inputs[binding] = lin;
	} break;

	case layout_out_flat:
	case layout_out_noperspective:
	case layout_out_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lout(fidx, qualifier.underlying, qualifier.kind);
		globals.outputs[binding] = lout;
	} break;

	case uniform:
	{
		size_t binding = qualifier.numerical;
		local_layout_type un(fidx, qualifier.underlying, qualifier.kind);
		globals.uniforms[binding] = un;
	} break;

	case storage_buffer:
	case read_only_storage_buffer:
	case write_only_storage_buffer:
	{
		size_t binding = qualifier.numerical;
		local_layout_type bf(fidx, qualifier.underlying, qualifier.kind);
		globals.buffers[binding] = bf;
	} break;

	case shared:
	{
		size_t id = qualifier.numerical;
		local_layout_type bf(fidx, qualifier.underlying, qualifier.kind);
		globals.shared[id] = bf;
	} break;

	case task_payload:
	{
		globals.special[task_payload][-1] = special_type(fidx, bidx);
		extensions.insert("GL_EXT_mesh_shader");
	} break;

	case write_only:
	case read_only:
	{
		// TODO: method to search
		auto &lower = function.atoms[qualifier.underlying];

		if (auto decl = lower.get <Qualifier> ()) {
			if (image_kind(decl->kind)) {
				globals.images[decl->numerical].extra.insert(qualifier.kind);
				break;
			}
		}

		JVL_ABORT("unexpected atom for {} qualifier:\n{}",
			tbl_qualifier_kind[qualifier.kind],
			lower);
	} break;
	
	case ray_tracing_payload:
	case ray_tracing_payload_in:
		globals.special[qualifier.kind][qualifier.numerical] = special_type(fidx, bidx);
		break;

	case glsl_intrinsic_gl_SubgroupInvocationID:
		extensions.insert("GL_KHR_shader_subgroup_basic");
		break;

	// Miscellaneous
	case arrays:
		break;

	case glsl_intrinsic_gl_GlobalInvocationID:
	case glsl_intrinsic_gl_InstanceIndex:
	case glsl_intrinsic_gl_LocalInvocationID:
	case glsl_intrinsic_gl_LocalInvocationIndex:
	case glsl_intrinsic_gl_MeshVerticesEXT:
	case glsl_intrinsic_gl_Position:
	case glsl_intrinsic_gl_PrimitiveTriangleIndicesEXT:
	case glsl_intrinsic_gl_VertexIndex:
	case glsl_intrinsic_gl_WorkGroupID:
	case glsl_intrinsic_gl_WorkGroupSize:
		break;

	default:
		JVL_WARNING("unhandled qualifier in function @{}:\n{}", bidx, qualifier);
		break;
	}
}

void LinkageUnit::process_function_intrinsic(Function &function, size_t index, Index i, const Intrinsic &intr)
{
	switch (intr.opn) {

	case thunder::layout_local_size:
	{
		thunder::Index args = intr.args;

		glm::uvec3 size = glm::uvec3(1, 1, 1);

		int32_t j = 0;
		while (args != -1 && j < 3) {
			Atom local = function[args];

			JVL_ASSERT_PLAIN(local.is <List> ());
			auto list = local.as <List> ();
			local = function[list.item];

			JVL_ASSERT_PLAIN(local.is <Primitive> ());
			auto value = local.as <Primitive> ();

			JVL_ASSERT_PLAIN(value.type == u32);
			size[j++] = value.udata;

			args = list.next;
		}

		local_size = size;
	} break;

	case thunder::layout_mesh_shader_sizes:
	{
		thunder::Index args = intr.args;

		glm::uvec2 size = glm::uvec2(1, 1);

		int32_t j = 0;
		while (args != -1 && j < 2) {
			Atom local = function[args];

			JVL_ASSERT_PLAIN(local.is <List> ());
			auto list = local.as <List> ();
			local = function[list.item];

			JVL_ASSERT_PLAIN(local.is <Primitive> ());
			auto value = local.as <Primitive> ();

			JVL_ASSERT_PLAIN(value.type == u32);
			size[j++] = value.udata;

			args = list.next;
		}

		mesh_shader_size = size;
	} break;

	case thunder::emit_mesh_tasks:
	case thunder::set_mesh_outputs:
		extensions.insert("GL_EXT_mesh_shader");
		break;

	case thunder::glsl_subgroupShuffle:
		extensions.insert("GL_KHR_shader_subgroup_shuffle");
		break;
	
	default:
		break;
	}
}

void LinkageUnit::process_function_aggregate(TypeMap &map, const Function &function, size_t index, Index i, QualifiedType qt)
{
	std::vector <Field> fields;

	fields.emplace_back(qt.as <StructFieldType> ().base(), "f0");

	do {
		auto &sft = qt.as <StructFieldType> ();
		auto name = fmt::format("f{}", fields.size());
		qt = function.types[sft.next];
		if (qt.is <StructFieldType> ()) {
			auto &sft = qt.as <StructFieldType> ();
			fields.emplace_back(sft.base(), name);
		} else {
			fields.emplace_back(qt, name);
		}
	} while (qt.is <StructFieldType> ());

	// Check for type hints
	size_t size = aggregates.size();
	
	std::string name = fmt::format("s{}_t", size);
	if (function.used_decorations.contains(i)) {
		auto &id = function.used_decorations.at(i);
		auto &decoration = function.decorations.at(id);
		name = decoration.name;

		for (size_t i = 0; i < fields.size(); i++)
			fields[i].name = decoration.fields[i];
	}

	map[i] = new_aggregate(index, name, fields);
}

// TODO: return referenced callables
std::set <Index> LinkageUnit::process_function(const Function &ftn)
{
	std::set <Index> referenced;

	// TODO: run validation here as well
	Index fidx = functions.size();

	functions.emplace_back(ftn);
	maps.emplace_back();

	auto &function = functions.back();
	auto &map = maps.back();

	for (Index bidx : function.synthesized) {
		// Sanity check
		JVL_ASSERT(bidx >= 0, "bad index @{} in synthesized list for function", bidx);

		auto &atom = function[bidx];

		// Get the return type of the function
		if (atom.is <Returns> ())
			function.returns = function.types[bidx];

		// Parameters and global variables
		if (atom.is <Qualifier> ())
			process_function_qualifier(function, fidx, bidx, atom.as <Qualifier> ());

		// Referenced callables
		if (atom.is <Call> ()) {
			auto &call = atom.as <Call> ();
			referenced.insert(call.cid);
		}

		// Checking for special intrinsics
		if (atom.is <Intrinsic> ())
			process_function_intrinsic(function, fidx, bidx, atom.as <Intrinsic> ());

		// Checking for structs used by the function
		auto qt = function.types[bidx];
		if (qt.is <StructFieldType> ())
			process_function_aggregate(map, function, fidx, bidx, qt);

		// TODO: static initializers (e.g. arrays and constants)
	}

	// Mark the dependencies
	dependencies[fidx] = referenced;
	cids[ftn.cid] = fidx;

	return referenced;
}

void LinkageUnit::add(const TrackedBuffer &callable)
{
	if (loaded.contains(callable.cid))
		return;

	Function converted {
		callable,
		callable.name,
		callable.cid
	};

	auto referenced = process_function(converted);

	loaded.insert(callable.cid);
	for (Index i : referenced)
		add(*TrackedBuffer::search_tracked(i));
}

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
using generator_list = std::vector <detail::c_like_generator_t>;

auto LinkageUnit::configure_generators() const
{
	generator_list generators;

	for (size_t i = 0; i < functions.size(); i++) {
		auto &function = functions[i];
		auto &map = maps[i];

		std::map <Index, std::string> structs;
		for (auto &[k, v] : map)
			structs[k] = aggregates[v].name;

		generators.emplace_back(detail::auxiliary_block_t(function, structs));
	}

	return generators;
}

// Generating the aggregates
void generate_aggregates(std::string &result,
			 const generator_list &generators,
			 const std::vector <Aggregate> &aggregates)
{
	for (auto &aggregate : aggregates) {
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
			    push_constant_info pc)
{
	if (pc.index == -1)
		return;

	auto &types = functions[pc.function].types;
	auto &generator = generators[pc.function];

	auto ts = generator.type_to_string(types[pc.index]);

	result += "layout (push_constant) uniform block\n";
	result += "{\n";
	if (pc.offset == 0)
		result += fmt::format("    {} _pc;\n", ts.pre + ts.post);
	else
		result += fmt::format("    layout (offset = {}) {} _pc;\n", pc.offset, ts.pre + ts.post);
	result += "};\n\n";
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

static std::string buffer_qualifier(QualifierKind kind)
{
	switch (kind) {
	case storage_buffer:
		return "";
	case read_only_storage_buffer:
		return "readonly ";
	case write_only_storage_buffer:
		return "writeonly ";
	default:
		break;
	}

	return "<buffer:?>";
}

void generate_buffers(std::string &result,
		     const generator_list &generators,
		     const std::vector <Function> &functions,
		     const auto &list)
{
	for (auto &[b, llt] : list) {
		auto &types = functions[llt.function].types;
		auto &generator = generators[llt.function];

		auto ts = generator.type_to_string(types[llt.index]);

		result += fmt::format("layout (binding = {}) {}buffer bblock{}\n",
			b, buffer_qualifier(llt.kind), b);
		result += "{\n";
		result += fmt::format("    {} _buffer{}{};\n", ts.pre, b, ts.post);
		result += "};\n\n";
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

void generate_images(std::string &result, const auto &images)
{
	for (const auto &[binding, llt] : images) {
		std::string modifier = " ";
		for (auto &k : llt.extra) {
			if (k == write_only)
				modifier += "writeonly ";
			if (k == read_only)
				modifier += "readonly ";
		}

		result += fmt::format("layout (binding = {}) uniform{}{} _image{};\n",
			binding,
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
		result += fmt::format("layout (binding = {}) uniform {} _sampler{};\n",
			binding, sampler_string(llt.kind), binding);
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
		switch (kind) {
		
		case task_payload:
		{
			auto &st = maps.at(-1);
			auto &types = functions[st.function].types;
			auto &generator = generators[st.function];
			auto ts = generator.type_to_string(types[st.index]);
			result += fmt::format("taskPayloadSharedEXT {} _task_payload;\n\n", ts.pre + ts.post);
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
				result += fmt::format("location (layout = {}) {} {} _ray_payload{}{};\n",
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

void generate_function(std::string &result,
		       detail::c_like_generator_t &generator,
		       const Function &function)
{
	auto ts = generator.type_to_string(function.returns);

	std::string args;
	for (size_t j = 0; j < function.args.size(); j++) {
		auto ts = generator.type_to_string(function.args[j]);
		args += fmt::format("{} _arg{}{}", ts.pre, j, ts.post);
		if (j + 1 < function.args.size())
			args += ", ";
	}

	result += fmt::format("{} {}({})\n",
		ts.pre + ts.post,
		function.name, args);

	result += "{\n";
	result += generator.generate();
	result += "}\n";
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

	// Globals: layout inputs and outputs
	generate_layout_io(result, generators, functions, "in", globals.inputs);
	generate_layout_io(result, generators, functions, "out", globals.outputs);

	// Globals: push constants
	generate_push_constant(result, generators, functions, globals.push_constant);

	// Globals: uniform variables and buffers
	generate_uniforms(result, generators, functions, globals.uniforms);
	generate_buffers(result, generators, functions, globals.buffers);

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

/////////////////////////////////
// Generation: C++ source code //
/////////////////////////////////

// TODO: separate source...

// Constructing primitive types in C++
template <typename T>
static std::string cpp_primitive_type_as_string()
{
	if constexpr (std::is_same_v <T, bool>)
		return "bool";

	if constexpr (std::is_same_v <T, int32_t>)
		return "int32_t";

	if constexpr (std::is_same_v <T, uint32_t>)
		return "uint32_t";

	if constexpr (std::is_same_v <T, float>)
		return "float";

	fmt::println("failed to generate primitive type as string");
	abort();
}

template <typename T, size_t N>
static std::string cpp_vector_type_as_string(const std::string &name)
{
	static const std::string components[] = { "x", "y", "z", "w" };

	std::string ret = fmt::format("struct {} {{\n", name);
	for (size_t i = 0; i < N; i++) {
		ret += fmt::format("    {} {};\n",
			cpp_primitive_type_as_string <T> (),
			components[i]);
	};

	// Default constructor
	ret += "\n";
	ret += fmt::format("    {}() = default;\n", name);

	// Constructor from elements
	ret += "\n";

	std::string args;
	for (size_t i = 0; i < N; i++) {
		args += fmt::format("{} arg{}",
			cpp_primitive_type_as_string <T> (), i);

		if (i + 1 < N)
			args += ", ";
	}

	ret += fmt::format("    {}({})\n", name, args);

	// Delegation list
	std::string list;

	for (size_t i = 0; i < N; i++) {
		list += fmt::format("{}(arg{})",
				components[i], i);

		if (i + 1 < N)
			list += ", ";
		else
			list += " {}";
	}

	ret += fmt::format("        : {}\n", list);

	return ret + "};\n\n";
}

static std::string cpp_primitive_type_as_string(PrimitiveType type)
{
	switch (type) {
	case i32:
	case u32:
	case f32:
	case boolean:
		return "";

	case ivec2:
		return cpp_vector_type_as_string <int32_t, 2> ("ivec2");
	case ivec3:
		return cpp_vector_type_as_string <int32_t, 3> ("ivec3");
	case ivec4:
		return cpp_vector_type_as_string <int32_t, 4> ("ivec4");

	case uvec2:
		return cpp_vector_type_as_string <uint32_t, 2> ("uvec2");
	case uvec3:
		return cpp_vector_type_as_string <uint32_t, 3> ("uvec3");
	case uvec4:
		return cpp_vector_type_as_string <uint32_t, 4> ("uvec4");

	case vec2:
		return cpp_vector_type_as_string <float, 2> ("vec2");
	case vec3:
		return cpp_vector_type_as_string <float, 3> ("vec3");
	case vec4:
		return cpp_vector_type_as_string <float, 4> ("vec4");

	default:
		break;
	}

	JVL_ABORT("failed to generate type string for primitive {}", tbl_primitive_types[type]);
}

auto used_primitives(const std::vector <Function> &functions)
{
	std::set <PrimitiveType> used_primitives;
	for (auto &function : functions) {
		for (auto &qt : function.types) {
			if (!qt.is <PlainDataType> ())
				continue;

			auto &pd = qt.as <PlainDataType> ();
			if (auto p = pd.get <PrimitiveType> ())
				used_primitives.insert(*p);
		}
	}

	return used_primitives;
}

std::string LinkageUnit::generate_cpp() const
{
	std::string result;

	// Add the necessary headers
	result += "#include <cstdint>\n";
	result += "\n";

	// Collect and synthesize all the primitives that are used
	for (auto p : used_primitives(functions)) {
		auto s = cpp_primitive_type_as_string(p);
		if (s.size()) {
			result += s;
			result += "\n";
		}
	}

	// Create the generators
	auto generators = configure_generators();

	// Generate each of the functions
	for (size_t i = 0; i < functions.size(); i++) {
		auto &function = functions[i];
		auto &generator = generators[i];

		generate_function(result, generator, function);

		if (i + 1 < functions.size())
			result += "\n";
	}

	return result;
}

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

std::vector <uint32_t> LinkageUnit::generate_spirv(const vk::ShaderStageFlagBits &flags) const
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

	if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
		std::string log = shader.getInfoLog();
		JVL_ERROR("failed to compile to SPIRV\n{}", log);
		return {};
	} else {
	}

	// Link the program
	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages)) {
		std::string log = program.getInfoLog();
		JVL_ERROR("failed to compile to SPIRV\n{}", log);
		return {};
	}

	std::vector <uint32_t> spirv;
	{
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &options);
	}
	return spirv;
}

//////////////////////////////////////////
// Generation: JIT compilation with GCC //
//////////////////////////////////////////

void *LinkageUnit::jit() const
{
	JVL_INFO("compiling linkage atoms with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	JVL_ASSERT(context, "failed to acquire context");

	// TODO: pass options
	gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);
	gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DEBUGINFO, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_TREE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	for (auto &function : functions) {
		// detail::unnamed_body_t body(block);
		detail::gcc_jit_function_generator_t generator(context, function);
		generator.generate();
	}

	gcc_jit_context_dump_to_file(context, "gcc_jit_result.c", true);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	JVL_ASSERT(result, "failed to compile function");

	void *ftn = gcc_jit_result_get_code(result, "function");
	JVL_ASSERT(result, "failed to load function result");

	JVL_INFO("successfully JIT-ed linkage unit");

	return ftn;
}

} // namespace jvl::thunder
