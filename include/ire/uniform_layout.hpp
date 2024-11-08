#pragma once

#include "../logging.hpp"
#include "../thunder/atom.hpp"
#include "aggregate.hpp"
#include "tagged.hpp"
#include "emitter.hpp"
#include "string_literal.hpp"

namespace jvl::ire {

// TODO: detail namespace...

// Ordinary layouts
struct layout_field {
	const char *name;
	bool aggregate;
	void *ptr;

	void ref_with(cache_index_t);
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout_t {
	std::string name;
	std::vector <layout_field> fields;

	// TODO: move down...
	void ref_with(cache_index_t up) {
		MODULE(uniform-layout-ref-with);
		
		auto &em = Emitter::active;

		// TODO: name hints in the active emitter...
		wrapped::tree <const void *, int> listed;

		for (size_t i = 0; i < fields.size(); i++) {
			layout_field f = fields[i];

			bool unique = !listed.contains(f.ptr);
			JVL_ASSERT(unique,
				"duplicate member in uniform layout, "
				"field #{} conflicts with field #{}",
				i + 1, listed[f.ptr] + 1);

			cache_index_t cit;
			cit = em.emit_load(up.id, i);
			f.ref_with(cit);

			listed[f.ptr] = i;
		}
	}
};

inline void layout_field::ref_with(cache_index_t up)
{
	if (aggregate) {
		using tmp_layout = uniform_layout_t <void>;
		auto ul = reinterpret_cast <tmp_layout *> (ptr);
		ul->ref_with(up);
	} else {
		auto t = reinterpret_cast <tagged *> (ptr);
		t->ref = up;
	}
}

// Const-qualified layouts
struct layout_const_field {
	bool aggregate;
	const char *name;
	const void *ptr;
};

template <typename T, typename ... Args>
thunder::index_t reconstruct_fields(size_t i, const std::vector <layout_const_field> &fields)
{
	MODULE(reconstruct);

	if (fields[i].aggregate)
		JVL_ABORT("unfinished implementation for reconstructing aggregates");

	thunder::List list;
	list.item = reinterpret_cast <const T *> (fields[i].ptr)->synthesize().id;

	if constexpr (sizeof...(Args))
		list.next = reconstruct_fields <Args...> (i + 1, fields);

	return Emitter::active.emit(list);
}

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct const_uniform_layout_t {
	std::string name;
	std::vector <layout_const_field> fields;

	// cache_index_t list() const;

	auto remove_const() const {
		uniform_layout_t <Args...> layout;
		layout.name = name;
		for (auto cf : fields) {
			layout_field f;
			f.aggregate = cf.aggregate;
			f.ptr = const_cast <void *> (cf.ptr);
			layout.fields.push_back(f);
		}

		return layout;
	}

	// Explicit (re)construction of an aggregate type
	thunder::index_t reconstruct() const {
		return reconstruct_fields <Args...> (0, fields);
	}
};

// Concept for ensuring that a struct can be synthesized
template <typename T>
concept aggregate = requires(T &ref, const T &cref) {
	{
		cref.layout().fields
	} -> std::same_as <std::vector <layout_const_field> &&>;
};

// Named fields
template <string_literal name, typename T>
struct __field {
	static constexpr string_literal literal = name;
	const tagged *ref;
	const void *agr;
	
};

// Concept for fields
template <typename T>
struct __field_type_t : std::false_type {};

template <string_literal name, typename T>
struct __field_type_t <__field <name, T>> : std::true_type {};

template <typename T>
concept __field_type = __field_type_t <T> ::value;

// Backend methods for creating uniform layouts
template <builtin T, string_literal name, typename ... UArgs>
void __const_init(layout_const_field *fields, int index, const __field <name, T> &t, const UArgs &... uargs)
{
	layout_const_field f;
	f.aggregate = false;
	f.name = name.value;
	f.ptr = t.ref;

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

template <aggregate T, string_literal name, typename ... UArgs>
void __const_init(layout_const_field *fields, int index, const __field <name, T> &t, const UArgs &... uargs)
{
	layout_const_field f;
	f.aggregate = true;
	f.name = name.value;
	f.ptr = t.agr;

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

template <string_literal_group field_group, typename ... Args>
struct higher_const_uniform_layout_t : const_uniform_layout_t <Args...> {
	static constexpr auto group = field_group;
};

template <aggregate T, string_literal field>
constexpr auto uniform_field_index()
{
	using layout_t = decltype(T().layout());
	using group_t = decltype(layout_t::group);
	constexpr int32_t i = group_t::find(field);
	static_assert(i != -1, "could not find field");
	if constexpr (i == -1)
		return aggregate_index <0> ();
	else
		return aggregate_index <i> ();
}

#define uniform_field(T, name) jvl::ire::uniform_field_index <T, #name> ()

} // namespace jvl::ire
