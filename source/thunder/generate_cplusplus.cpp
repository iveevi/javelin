#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "ire/callable.hpp"

namespace jvl::thunder {

// Generating C++ source code
template <typename T>
std::string cpp_primitive_type_as_string()
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
std::string jvl_vector_type_as_string(const std::string &name)
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

std::string jvl_primitive_type_as_string(PrimitiveType type)
{
	switch (type) {
	case i32:
	case u32:
	case f32:
	case boolean:
		return "";

	case ivec2:
		return jvl_vector_type_as_string <int32_t, 2> ("ivec2");
	case ivec3:
		return jvl_vector_type_as_string <int32_t, 3> ("ivec3");
	case ivec4:
		return jvl_vector_type_as_string <int32_t, 4> ("ivec4");

	case uvec2:
		return jvl_vector_type_as_string <uint32_t, 2> ("uvec2");
	case uvec3:
		return jvl_vector_type_as_string <uint32_t, 3> ("uvec3");
	case uvec4:
		return jvl_vector_type_as_string <uint32_t, 4> ("uvec4");

	case vec2:
		return jvl_vector_type_as_string <float, 2> ("vec2");
	case vec3:
		return jvl_vector_type_as_string <float, 3> ("vec3");
	case vec4:
		return jvl_vector_type_as_string <float, 4> ("vec4");

	default:
		break;
	}

	fmt::println("failed to generate type as string for {}", tbl_primitive_types[type]);
	abort();
}

std::string Linkage::generate_cplusplus()
{
	wrapped::hash_table <int, std::string> struct_names;

	// Translating types
	auto translate_type = [&](index_t i) -> std::string {
		const auto &decl = structs[i];
		if (decl.size() == 1) {
			auto &elem = decl[0];
			return tbl_primitive_types[elem.item];
		}

		return struct_names[i];
	};

	// Final generated source
	std::string source;

	// All the includes
	source += "#include <tuple>\n";
	source += "\n";

	// Structure declarations, should already be in order
	std::unordered_set <int> synthesized_primitives;

	for (index_t i = 0; i < structs.size(); i++) {
		const auto &decl = structs[i];

		assert(decl.size() >= 1);
		if (decl.size() == 1) {
			auto item = decl[0].item;

			// Already synthesized, no need to do it again
			if (synthesized_primitives.count(item))
				continue;

			// If we are here, it means that the type is a primitive
			source += jvl_primitive_type_as_string(item);
			synthesized_primitives.insert(item);

			continue;
		}

		std::string struct_name = fmt::format("s{}_t", i);
		std::string struct_source = fmt::format("struct {} {{\n", struct_name);
		struct_names[i] = struct_name;

		size_t field = 0;
		for (auto &elem : decl) {
			std::string ft_name;
			if (elem.nested == -1)
				ft_name = tbl_primitive_types[elem.item];
			else
				ft_name = struct_names[elem.nested];

			struct_source += fmt::format("    {} f{};\n", ft_name, field++);
		}

		struct_source += "};\n";

		source += struct_source + "\n";
	}

	// TODO: layout in/out and push constants are global variables
	// or const & passed values, depending on the user input

	// Synthesize all blocks
	for (auto &i : sorted) {
		auto &b = blocks[i];

		wrapped::hash_table <int, std::string> local_struct_names;
		for (auto &[i, t] : b.struct_map)
			local_struct_names[i] = translate_type(t);

		// Output type
		// TODO: respect callable return values as well...
		std::string returns;

		if (b.returns != -1) {
			returns = translate_type(b.returns);
		} else if (louts.size() == 0) {
			returns = "void";
		} else if (louts.size() == 1) {
			auto it = louts.begin();
			returns = translate_type(it->second);
		} else {
			size_t count = 0;
			for (auto [_, t] : louts) {
				returns += translate_type(t);
				if (count + 1 < louts.size()) {
					returns += ", ";
				}

				count++;
			}

			returns = "std::tuple <" + returns + ">";
		}

		// Function arguments
		std::vector <std::string> args;

		// For now, layout inputs are passed by const reference
		for (const auto &[binding, t] : lins)
			args.emplace_back(fmt::format("{} _lin{}", translate_type(t), binding));

		for (index_t i = 0; i < b.parameters.size(); i++) {
			index_t t = b.parameters.at(i);
			std::string arg = fmt::format("{} _arg{}", translate_type(t), i);
			args.push_back(arg);
		}

		std::string parameters;
		for (index_t i = 0; i < args.size(); i++) {
			parameters += args[i];
			if (i + 1 < args.size())
				parameters += ", ";
		}

		source += fmt::format("{} {}({})\n", returns, b.name, parameters);
		source += "{\n";

		// Declare storage for each layout outputs (return value)
		for (auto [binding, t] : louts)
			source += fmt::format("    {} _lout{};\n", translate_type(t), binding);
		
		auto synthesized = detail::synthesize_list(b.unit);

		source += detail::generate_body_c_like(b.unit,
			local_struct_names,
			synthesized,
			b.unit.size());

		// Synthesizing the return statement
		if (louts.size() == 1) {
			source += "    return _lout0;\n";
		} else if (louts.size() > 1) {
			size_t count = 0;

			// At the end, create the tuple and return it
			std::string returns;
			for (auto [binding, _] : louts) {
				returns += fmt::format("_lout{}", binding);
				if (count + 1 < louts.size())
					returns += ", ";

				count++;
			}

			source += fmt::format("    return std::make_tuple({});\n", returns);
		}

		source += "}\n\n";
	}

	return source;
}

} // namespace jvl::thunder