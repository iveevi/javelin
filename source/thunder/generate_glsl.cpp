#include <string>

#include "ire/callable.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

std::string Linkage::generate_glsl(const std::string &version)
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

	// Final source code
	std::string source = fmt::format("#version {}\n\n", version);

	// Structure declarations, should already be in order
	for (index_t i = 0; i < structs.size(); i++) {
		const auto &decl = structs[i];
		if (decl.size() <= 1)
			continue;

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

	// Global shader variables
	for (const auto &[binding, t] : lins) {
		source += fmt::format("layout (location = {}) in {} _lin{};\n",
				binding, translate_type(t), binding);
	}

	if (lins.size())
		source += "\n";

	for (const auto &[binding, t] : louts) {
		source += fmt::format("layout (location = {}) out {} _lout{};\n",
				binding, translate_type(t), binding);
	}

	if (louts.size())
		source += "\n";

	if (push_constant != -1) {
		source += "layout (push_constant) uniform constants\n";
		source += "{\n";
		source += fmt::format("     {} _pc;\n", translate_type(push_constant));
		source += "};\n";
		source += "\n";
	}

	// Synthesize all blocks
	for (auto &i : sorted) {
		auto &b = blocks[i];

		wrapped::hash_table <int, std::string> local_struct_names;
		for (auto &[i, t] : b.struct_map)
			local_struct_names[i] = translate_type(t);

		std::string name = "main";
		if (i != -1) {
			ire::Callable *cbl = ire::Callable::search_tracked(i);
			name = cbl->name;
		}

		std::string returns = "void";
		if (b.returns != -1)
			returns = translate_type(b.returns);

		std::vector <std::string> args;
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

		auto synthesized = detail::synthesize_list(b.unit);

		source += fmt::format("{} {}({})\n", returns, name, parameters);
		source += "{\n";

		source += detail::generate_body_c_like(b.unit,
			local_struct_names,
			synthesized,
			b.unit.size());

		source += "}\n\n";
	}

	return source;
}

} // namespace jvl::thunder