#include <map>
#include <type_traits>

#include "atom/kernel.hpp"
#include "atom/atom.hpp"
#include "wrapped_types.hpp"

namespace jvl::atom {

// Printing the IR stored in a kernel
// TODO: debug only?
void Kernel::dump()
{
	// TODO: more properties
	fmt::println("------------------------------");
	fmt::println("KERNEL:");
	fmt::println("    {} atoms", atoms.size());
	fmt::println("------------------------------");
	for (size_t i = 0; i < atoms.size(); i++) {
		if (synthesized.contains(i))
			fmt::print("S");
		else
			fmt::print(" ");

		if (used.contains(i))
			fmt::print("U");
		else
			fmt::print(" ");

		fmt::print(" [{:4d}]: ", i);
			atom::dump_ir_operation(atoms[i]);
		fmt::print("\n");
	}
}

// Generating structs
std::string synthesize_struct(const std::vector <atom::General> &atoms, int i,
		              wrapped::hash_table <int, std::string> &struct_names,
			      int &struct_index)
{
	atom::General g = atoms[i];
	if (!g.is <atom::TypeField>())
		return "";

	atom::TypeField tf = g.as <atom::TypeField>();
	if (tf.next == -1 && tf.down == -1)
		return "";

	// TODO: handle nested structs (down)
	std::string struct_name = fmt::format("s{}_t", struct_index++);

	struct_names[i] = struct_name;

	std::string struct_source;
	struct_source = fmt::format("struct {} {{\n", struct_name);

	int field_index = 0;
	int j = i;
	while (j != -1) {
		atom::General g = atoms[j];
		if (!g.is <atom::TypeField> ())
			abort();

		atom::TypeField tf = g.as <atom::TypeField> ();
		// TODO: nested
		// TODO: put this whole thing in a method

		struct_source += fmt::format("    {} f{};\n", atom::type_table[tf.item], field_index++);

		j = tf.next;
	}

	struct_source += "};\n\n";

	return struct_source;
}

// Gathering the input/output structure of the kernel
struct io_structure {
	std::map <int, std::string> lins;
	std::map <int, std::string> louts;
	std::string push_constant;

	static io_structure from(const std::vector <atom::General> &atoms,
			         const wrapped::hash_table <int, std::string> &struct_names,
				 const std::unordered_set <atom::index_t> &used) {
		io_structure ios;

		for (int i = 0; i < atoms.size(); i++) {
			if (!used.count(i))
				continue;

			auto op = atoms[i];
			if (!op.is <atom::Global> ())
				continue;

			auto global = std::get <atom::Global> (op);
			auto type = atom::type_name(atoms.data(), struct_names, i, -1);
			if (global.qualifier == atom::Global::layout_in)
				ios.lins[global.binding] = type;
			else if (global.qualifier == atom::Global::layout_out)
				ios.louts[global.binding] = type;
			else if (global.qualifier == atom::Global::push_constant)
				ios.push_constant = type;
		}

		return ios;
	}
};

// Generating GLSL source code
std::string Kernel::synthesize_glsl(const std::string &version_number)
{
	// Final generated source
	std::string source;

	// Version header
	source += "#version " + version_number + "\n";
	source += "\n";

	// Gather all necessary structs
	wrapped::hash_table <int, std::string> struct_names;

	int struct_index = 0;
	for (int i = 0; i < atoms.size(); i++) {
		if (!synthesized.contains(i))
			continue;

		source += synthesize_struct(atoms, i, struct_names, struct_index);
	}

	auto ios = io_structure::from(atoms, struct_names, used);

	// Global shader variables
	for (const auto &[binding, type] : ios.lins) {
		source += fmt::format("layout (location = {}) in {} _lin{};\n",
				binding, type, binding);
	}

	if (ios.lins.size())
		source += "\n";

	for (const auto &[binding, type] : ios.louts)
		source += fmt::format("layout (location = {}) out {} _lout{};\n",
				binding, type, binding);

	if (ios.louts.size())
		source += "\n";

	// TODO: check for vulkan or opengl, etc
	if (ios.push_constant.size()) {
		source += "layout (push_constant) uniform constants\n";
		source += "{\n";
		source += "     " + ios.push_constant + " _pc;\n";
		source += "}\n";
		source += "\n";
	}

	// Main function
	source += "void main()\n";
	source += "{\n";
	source += atom::synthesize_glsl_body(atoms.data(), struct_names, synthesized, atoms.size());
	source += "}\n";

	return source;
}

// Generating C++ source code
template <typename T>
std::string cpp_primitive_type_as_string()
{
	if constexpr (std::is_same_v <T, int>)
		return "int";

	if constexpr (std::is_same_v <T, float>)
		return "float";

	if constexpr (std::is_same_v <T, bool>)
		return "bool";

	return "?";
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

	return ret + "};\n\n";
}

std::string jvl_primitive_type_as_string(atom::PrimitiveType type)
{
	switch (type) {
	case i32:
	case f32:
	case boolean:
		return "";
	case vec2:
		return jvl_vector_type_as_string <float, 2> ("vec2");
	case vec3:
		return jvl_vector_type_as_string <float, 3> ("vec3");
	case vec4:
		return jvl_vector_type_as_string <float, 4> ("vec4");
	default:
		break;
	}

	return "?";
}

std::string Kernel::synthesize_cplusplus()
{
	// Final generated source
	std::string source;

	// Generate structs for the used primitive types
	// TODO: unless told to use jvl structures...
	for (int i = 0; i < atoms.size(); i++) {
		if (!used.contains(i))
			continue;

		atom::General g = atoms[i];
		if (!g.is <atom::TypeField>())
			continue;

		atom::TypeField tf = g.as <atom::TypeField>();
		if (tf.next != -1 || tf.down != -1)
			continue;

		// If we are here, it means that the type is a primitive
		source += jvl_primitive_type_as_string(tf.item);
	}

	// Gather all necessary structs
	wrapped::hash_table <int, std::string> struct_names;

	int struct_index = 0;
	for (int i = 0; i < atoms.size(); i++) {
		if (!synthesized.contains(i))
			continue;

		source += synthesize_struct(atoms, i, struct_names, struct_index);
	}

	auto ios = io_structure::from(atoms, struct_names, used);

	// TODO: name hint for layout output structure (as a replacement for a tuple)

	// Input signature for the function
	std::vector <std::string> parameters;
	for (const auto &[binding, type] : ios.lins)
		parameters.emplace_back(fmt::format("{} _lin{}", type, binding));

	// Output type
	std::string return_type;

	if (ios.louts.size() == 1) {
		auto it = ios.louts.begin();
		return_type = it->second;
	} else {
		size_t count = 0;
		for (auto [_, type] : ios.louts) {
			return_type += type;
			if (count + 1 < ios.louts.size()) {
				return_type += ", ";
			}

			count++;
		}

		return_type = "std::tuple <" + return_type + ">";
	}

	// Synthesize the function signature
	std::string parameter_string;

	if (ios.push_constant.size()) {
		parameter_string += fmt::format("const {} &_pc", ios.push_constant);
		if (ios.lins.size())
			parameter_string += ", ";
	}

	for (size_t i = 0; i < parameters.size(); i++) {
		parameter_string += parameters[i];
		if (i + 1 < parameters.size())
			parameter_string += ", ";
	}

	source += return_type + " kernel(" + parameter_string + ")\n";

	// Fill in the outputs
	source += "{\n";

	for (auto [binding, type] : ios.louts)
		source += fmt::format("    {} _lout{};\n", type, binding);

	// Process the body of the function
	source += atom::synthesize_glsl_body(atoms.data(), struct_names, synthesized, atoms.size());

	// At the end, create the tuple and return it
	std::string returns;

	size_t count = 0;
	for (auto [binding, _] : ios.louts) {
		returns += fmt::format("_lout{}", binding);
		if (count + 1 < ios.louts.size()) {
			returns += ", ";
		}

		count++;
	}

	source += fmt::format("    return std::make_tuple({})\n", returns);

	source += "}\n";

	return source;
}

} // namespace jvl::atom
