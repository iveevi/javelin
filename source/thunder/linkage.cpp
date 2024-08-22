#include "thunder/linkage.hpp"
#include "ire/callable.hpp"

namespace jvl::thunder {

bool compare_declaration(const Linkage::struct_declaration &A,
			 const Linkage::struct_declaration &B)
{
	if (A.size() != B.size())
		return false;

	for (size_t i = 0; i < A.size(); i++) {
		auto &Ae = A[i];
		auto &Be = B[i];

		if (Ae.item != Be.item || Ae.nested != Be.nested)
			return false;
	}

	return true;
}

index_t Linkage::insert(const struct_declaration &A)
{
	auto finder = [&](const Linkage::struct_declaration &B) {
		return compare_declaration(A, B);
	};

	auto it = std::find_if(structs.begin(), structs.end(), finder);
	if (it == structs.end()) {
		index_t s = structs.size();
		structs.push_back(A);
		return s;
	}

	return std::distance(structs.begin(), it);
}

// Fully resolve the linkage
Linkage &Linkage::resolve()
{
	// If there are no callables used, there is nothing to resolve
	if (callables.empty())
		return *this;

	sorted.clear();

	// Get the resolved linkages of the used callables
	wrapped::hash_table <index_t, Linkage> used;
	for (auto &i : callables) {
		ire::Callable *cbl = ire::Callable::search_tracked(i);

		Kernel cbl_kernel = cbl->export_to_kernel();

		Linkage linkage = cbl_kernel.linkage();
		linkage.resolve();

		for (auto [j, b] : linkage.blocks) {
			struct_map_t map;
			for (auto &[k, v] : b.struct_map)
				map[k] = insert(linkage.structs[v]);

			index_t k = (j == -1) ? i : j;
			blocks[k].synthesized = b.synthesized;
			blocks[k].struct_map = map;
			blocks[k].unit = b.unit;

			blocks[k].returns = b.returns;
			if (b.returns != -1)
				blocks[k].returns = insert(linkage.structs[b.returns]);

			for (auto &[binding, t] : b.parameters)
				blocks[k].parameters[binding] = insert(linkage.structs[t]);
		}

		// Add dependencies in order
		for (auto j : linkage.sorted) {
			if (j == -1)
				j = i;

			sorted.push_back(j);
		}

		// TODO: combine lins and louts (not parameters though)
	}

	sorted.push_back(-1);

	return *this;
}

std::string Linkage::synthesize_glsl(const std::string &version)
{
	wrapped::hash_table <int, std::string> struct_names;

	// Final source code
	std::string source = fmt::format("#version {}\n\n", version);

	// Translating types
	auto translate_type = [&](index_t i) -> std::string {
		const auto &decl = structs[i];
		if (decl.size() == 1) {
			auto &elem = decl[0];
			return type_table[elem.item];
		}

		return struct_names[i];
	};

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
				ft_name = type_table[elem.item];
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

		source += fmt::format("{} {}({})\n", returns, name, parameters);
		source += "{\n";
		source += synthesize_glsl_body(b.unit.data(),
				               local_struct_names,
					       b.synthesized,
					       b.unit.size());
		source += "}\n\n";
	}

	return source;
}

// Printing the linkage model
void Linkage::dump() const
{
	fmt::println("------------------------------");
	fmt::println("LINKAGE:");
	fmt::println("------------------------------");

	fmt::println("Callables referenced: {}", callables.size());
	for (auto &i : callables) {
		ire::Callable *cbl = ire::Callable::search_tracked(i);
		fmt::println("  ${} (@{})", cbl->name, i);
	}

	fmt::println("Layout inputs: {}", lins.size());
	for (auto &[i, t] : lins)
		fmt::println("  @{} -> %{}", i, t);

	fmt::println("Layout outputs: {}", louts.size());
	for (auto &[i, t] : louts)
		fmt::println("  @{} -> %{}", i, t);

	fmt::print("Push constant: ");
	if (push_constant >= 0)
		fmt::println("%{}", push_constant);
	else
		fmt::println("(nil)");

	fmt::println("Structure declarations: {}", structs.size());
	for (auto &decl : structs) {
		fmt::println("  struct with {} elements", decl.size());
		for (auto &element : decl) {
			if (element.nested >= 0)
				fmt::println("    -> %{}", element.nested);
			else
				fmt::println("    -> {}", type_table[element.item]);
		}
	}

	fmt::println("Blocks: {}", blocks.size());
	for (auto &[i, b] : blocks) {
		if (i == -1) {
			fmt::println("  Block $main:");
		} else {
			ire::Callable *cbl = ire::Callable::search_tracked(i);
			fmt::println("  Block ${}:", cbl->name);
		}

		fmt::println("    synthesized: {}", b.synthesized.size());
		fmt::println("    instructions: {}", b.unit.size());
		fmt::println("    returns: %{}", b.returns);

		fmt::println("    parameters: {}", b.parameters.size());
		for (auto &[i, t] : b.parameters)
			fmt::println("      @{} -> %{}", i, t);

		fmt::println("    local struct mapping:");
		for (auto &[i, t] : b.struct_map)
			fmt::println("      {} -> %{}", i, t);
	}

	fmt::println("Dependency order:");
	for (auto &i : sorted) {
		if (i == -1) {
			fmt::print("$main ");
		} else {
			ire::Callable *cbl = ire::Callable::search_tracked(i);
			fmt::print("${} ", cbl->name);
		}
	}
	fmt::print("\n");
}

} // namespace jvl::atom
