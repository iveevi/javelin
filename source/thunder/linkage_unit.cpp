#include <libgccjit.h>

#include "thunder/c_like_generator.hpp"
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
Function::Function(const Buffer &buffer, const std::string &name_)
		: Buffer(buffer), name(name_) {}

// Linkage unit methods
index_t LinkageUnit::new_aggregate(size_t ftn, const std::vector <QualifiedType> &fields)
{
	size_t index = aggregates.size();

	Aggregate aggr {
		.function = ftn,
		.name = fmt::format("s{}_t", index),
		.fields = fields
	};

	auto it = std::find(aggregates.begin(), aggregates.end(), aggr);
	if (it == aggregates.end()) {
		aggregates.push_back(aggr);
		return index;
	}

	return std::distance(aggregates.begin(), it);
}

void LinkageUnit::process_function_qualifier(Function &function, index_t index, index_t i, const Qualifier &qualifier)
{
	if (sampler_kind(qualifier.kind)) {
		size_t binding = qualifier.numerical;
		globals.samplers[binding] = qualifier.kind;
	}

	switch (qualifier.kind) {

	case parameter:
	{
		size_t binding = qualifier.numerical;
		size_t size = std::max(function.args.size(), binding + 1);
		function.args.resize(size);
		function.args[binding] = function.types[i];
	} break;

	case push_constant:
	{
		globals.push_constant = {
			(size_t) index,
			i,
			push_constant
		};
	} break;

	case layout_in_flat:
	case layout_in_noperspective:
	case layout_in_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lin(index, qualifier.underlying, qualifier.kind);
		globals.inputs[binding] = lin;
	} break;

	case layout_out_flat:
	case layout_out_noperspective:
	case layout_out_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lout(index, qualifier.underlying, qualifier.kind);
		globals.outputs[binding] = lout;
	} break;

	default:
		break;
	}
}

void LinkageUnit::process_function_aggregate(TypeMap &map, const Function &function, index_t i, QualifiedType qt)
{
	std::vector <QualifiedType> fields {
		qt.as <StructFieldType> ().base()
	};

	do {
		auto &sft = qt.as <StructFieldType> ();
		qt = function.types[sft.next];
		if (qt.is <StructFieldType> ()) {
			auto &sft = qt.as <StructFieldType> ();
			fields.push_back(sft.base());
		} else {
			// TODO: if its concrete, we need to reference it...
			JVL_ASSERT_PLAIN(qt.is_primitive());
			fields.push_back(qt);
		}
	} while (qt.is <StructFieldType> ());

	map[i] = new_aggregate(i, fields);
}

// TODO: return referenced callables
std::vector <index_t> LinkageUnit::process_function(const Function &ftn)
{
	std::vector <index_t> referenced;

	// TODO: run validation here as well
	index_t index = functions.size();

	functions.emplace_back(ftn);
	maps.emplace_back();

	auto &function = functions.back();
	auto &map = maps.back();

	for (size_t i : function.synthesized) {
		auto &atom = function[i];

		// Get the return type of the function
		if (atom.is <Returns> ())
			function.returns = function.types[i];

		// Parameters and global variables
		if (atom.is <Qualifier> ())
			process_function_qualifier(function, index, i, atom.as <Qualifier> ());

		// Referenced callables
		if (atom.is <Call> ()) {
			auto &call = atom.as <Call> ();
			referenced.push_back(call.cid);
		}

		// Checking for structs used by the function
		auto qt = function.types[i];
		if (qt.is <StructFieldType> ())
			process_function_aggregate(map, function, i, qt);
	}

	return referenced;
}

void LinkageUnit::add(const TrackedBuffer &callable)
{
	if (loaded.contains(callable.cid))
		return;

	auto referenced = process_function(Function(callable, callable.name));

	loaded.insert(callable.cid);
	for (index_t i : referenced)	
		add(*TrackedBuffer::search_tracked(i));
}

//////////////////////////////////
// Generation: GLSL source code //
//////////////////////////////////

// Helper methods
static std::string layout_interpolationstring(QualifierKind kind)
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

		std::map <index_t, std::string> structs;
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
		auto &generator = generators[aggregate.function];

		result += "struct " + aggregate.name + " {\n";
		for (size_t i = 0; i < aggregate.fields.size(); i++) {
			auto ts = generator.type_to_string(aggregate.fields[i]);
			result += fmt::format("\t{} f{}{};\n", ts.pre, i, ts.post);
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
		auto interpolation = layout_interpolationstring(llt.kind);

		result += fmt::format("layout (location = {}) {} {}{} _l{}{};\n",
			b, iot, interpolation, ts.pre + ts.post, iot, b);
	}

	if (list.size())
		result += "\n";
}

void generat_push_constant(std::string &result,
			   const generator_list &generators,
			   const std::vector <Function> &functions,
			   local_layout_type pc)
{
	if (pc.index == -1)
		return;
		
	auto &types = functions[pc.function].types;
	auto &generator = generators[pc.function];

	auto ts = generator.type_to_string(types[pc.index]);

	result += "layout (push_constant) uniform block\n";
	result += "{\n";
	result += fmt::format("\t{} _pc;\n", ts.pre + ts.post);
	result += "};\n\n";
}

void generat_samplers(std::string &result,
		      const auto &samplers)
{
	for (const auto &[binding, kind] : samplers) {
		result += fmt::format("layout (binding = {}) uniform {} _sampler{};\n",
			binding, sampler_string(kind), binding);
	}

	if (samplers.size())
		result += "\n";
}

void generate_function(std::string &result,
		       detail::c_like_generator_t &generator,
		       const Function &function)
{
	auto ts = generator.type_to_string(function.returns);

	// TODO: topologically sort the functions before synthesis

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

// Primary generation routine
std::string LinkageUnit::generate_glsl() const
{
	std::string result;

	result += "#version 450\n";
	result += "\n";

	// Create the generators
	auto generators = configure_generators();

	// Declare the aggregates used
	generate_aggregates(result, generators, aggregates);

	// Globals: layout inputs and outputs
	generate_layout_io(result, generators, functions, "in", globals.inputs);
	generate_layout_io(result, generators, functions, "out", globals.outputs);
	
	// Globals: push constants
	generat_push_constant(result, generators, functions, globals.push_constant);

	// Globals: samplers
	generat_samplers(result, globals.samplers);

	// Generate each of the functions
	for (size_t i = 0; i < functions.size(); i++) {
		auto &function = functions[i];
		auto &generator = generators[i];
			
		// TODO: topologically sort the functions before synthesis
		generate_function(result, generator, function);

		if (i + 1 < functions.size())
			result += "\n";
	}

	return result;
}

/////////////////////////////////
// Generation: C++ source code //
/////////////////////////////////

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