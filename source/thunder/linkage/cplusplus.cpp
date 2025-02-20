#include "thunder/enumerations.hpp"
#include "thunder/linkage_unit.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

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

} // namespace jvl::thunder
