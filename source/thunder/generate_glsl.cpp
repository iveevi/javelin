#include <string>

#include "ire/callable.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/c_like_generator.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "thunder/properties.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

MODULE(generate-glsl);

std::string layout_interpolation_string(QualifierKind kind)
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

std::string sampler_string(QualifierKind kind)
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

std::string Linkage::generate_glsl(const profiles::glsl_version &glsl_version)
{
	wrapped::hash_table <int, std::string> struct_names;

	// Translating types
	auto translate_type = [&](index_t i) -> std::string {
		JVL_ASSERT(i < (index_t) structs.size(),
			"index {} is out of range of recorded "
			"structs (with {} elements)", i, structs.size());

		const auto &decl = structs[i];
		if (decl.size() == 1) {
			auto &elem = decl[0];
			return tbl_primitive_types[elem.item];
		}

		return struct_names[i];
	};

	// Final source code
	std::string source = fmt::format("#version {}\n\n", glsl_version.version);

	// Structure declarations, should already be in order
	for (size_t i = 0; i < structs.size(); i++) {
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
	for (const auto &[binding, linfo] : lins) {
		std::string location;
		if (glsl_version.explicit_location)
			location = fmt::format("layout (location = {}) ", binding);

		source += fmt::format("{}in {}{} _lin{};\n",
			location,
			layout_interpolation_string(linfo.kind),
			translate_type(linfo.type),
			binding);
	}

	if (lins.size())
		source += "\n";

	for (const auto &[binding, linfo] : louts) {
		std::string location;
		if (glsl_version.explicit_location)
			location = fmt::format("layout (location = {}) ", binding);
		
		source += fmt::format("{}out {}{} _lout{};\n",
			location,
			layout_interpolation_string(linfo.kind),
			translate_type(linfo.type),
			binding);
	}

	if (louts.size())
		source += "\n";

	// Samplers
	for (const auto &[binding, kind] : samplers) {
		source += fmt::format("layout (binding = {}) uniform {} _sampler{};\n",
			binding, sampler_string(kind), binding);
	}

	if (samplers.size())
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

		std::string returns = "void";
		if (b.returns != -1) {
			index_t t = b.struct_map[b.returns];
			returns = translate_type(t);
		}

		std::vector <std::string> args;
		for (size_t i = 0; i < b.parameters.size(); i++) {
			index_t t = b.parameters.at(i);
			t = b.struct_map.at(t);
			std::string arg = fmt::format("{} _arg{}", translate_type(t), i);
			args.push_back(arg);
		}

		std::string parameters;
		for (size_t i = 0; i < args.size(); i++) {
			parameters += args[i];
			if (i + 1 < args.size())
				parameters += ", ";
		}

		detail::auxiliary_block_t aux(b, local_struct_names);
		detail::c_like_generator_t generator(aux);

		// TODO: toggle naming specs depending on kernel flags
		source += fmt::format("{} {}({})\n", returns, "main", parameters);
		source += "{\n";
		source += generator.generate();
		source += "}\n\n";
	}

	return source;
}

} // namespace jvl::thunder