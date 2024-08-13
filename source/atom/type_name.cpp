#include "atom/atom.hpp"

namespace jvl::atom {

std::string type_name(const General *const pool,
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

	General g = pool[index];
	if (auto global = g.get <Global> ()) {
		return type_name(pool, struct_names, global->type, field);
	} else if (auto tf = g.get <TypeField> ()) {
		if (tf->down != -1)
			return type_name(pool, struct_names, tf->down, -1);
		if (tf->item != bad)
			return type_table[tf->item];
	}

	return "<BAD>";
}

} // namespace jvl::atom
