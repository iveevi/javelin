#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::thunder {

std::string type_name
(
	const std::vector <Atom> &pool,
	const wrapped::hash_table <int, std::string> &struct_names,
	index_t index,
	index_t field
)
{
	Atom g = pool[index];
	if (struct_names.contains(index)) {
		if (field == -1)
			return struct_names.at(index);

		assert(g.is <TypeField> ());
		if (field > 0) {
			index_t i = g.as <TypeField> ().next;
			// TODO: traverse inline
			return type_name(pool, struct_names, i, field - 1);
		}
	}

	if (auto global = g.get <Global> ()) {
		return type_name(pool, struct_names, global->type, field);
	} else if (auto ctor = g.get <Construct> ()) {
		return type_name(pool, struct_names, ctor->type, field);
	} else if (auto tf = g.get <TypeField> ()) {
		if (tf->down != -1)
			return type_name(pool, struct_names, tf->down, -1);
		if (tf->item != bad)
			return tbl_primitive_types[tf->item];
	} else {
		fmt::println("failed to resolve type name for");
		dump_ir_operation(g);
		fmt::print("\n");
	}

	return "<BAD>";
}

} // namespace jvl::atom
