#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::thunder {

std::string type_name(const Atom *const pool,
		      const wrapped::hash_table <int, std::string> &struct_names,
		      int index,
		      int field)
{
	if (struct_names.contains(index)) {
		if (field == -1)
			return struct_names.at(index);

		index -= std::max(0, field);
		field = 0;
	}

	Atom g = pool[index];
	if (auto global = g.get <Global> ()) {
		return type_name(pool, struct_names, global->type, field);
	} else if (auto tf = g.get <TypeField> ()) {
		if (tf->down != -1)
			return type_name(pool, struct_names, tf->down, -1);
		if (tf->item != bad)
			return tbl_primitive_types[tf->item];
	}

	return "<BAD>";
}

} // namespace jvl::atom
