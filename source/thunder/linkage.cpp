#include "thunder/linkage.hpp"
#include "ire/callable.hpp"
#include "thunder/enumerations.hpp"

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
			blocks[k].name = cbl->name;
			blocks[k].struct_map = map;
			blocks[k].unit = b.unit;

			blocks[k].returns = b.returns;
			if (b.returns != -1)
				blocks[k].returns = insert(linkage.structs[b.returns]);

			for (auto &[binding, t] : b.parameters)
				blocks[k].parameters[binding] = t;
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
	for (size_t i = 0; i < structs.size(); i++) {
		auto &decl = structs[i];

		fmt::println("  ({:2d}) struct with {} elements", i, decl.size());
		for (auto &element : decl) {
			if (element.nested >= 0)
				fmt::println("    -> %{}", element.nested);
			else
				fmt::println("    -> {}", tbl_primitive_types[element.item]);
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
			fmt::print("$origin ");
		} else {
			ire::Callable *cbl = ire::Callable::search_tracked(i);
			fmt::print("${} ", cbl->name);
		}
	}
	fmt::print("\n");
}

} // namespace jvl::thunder
