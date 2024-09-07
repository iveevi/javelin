#pragma once

#include <array>

#include "../logging.hpp"
#include "../thunder/atom.hpp"
#include "ire/aggregate.hpp"
#include "tagged.hpp"
#include "emitter.hpp"
#include "string_literal.hpp"

namespace jvl::ire {

MODULE(uniform-layout);

// Field classifications
enum {
	eField,
	eNested
};

// Ordinary layouts
struct layout_field {
	const char *name;
	int type;
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
		// If there is only one element then transfer it only
		if constexpr (sizeof...(Args) == 1) {
			layout_field f = fields[0];
			f.ref_with(up);
		} else {
			wrapped::hash_table <void *, int> listed;

			auto &em = Emitter::active;

			// TODO: name hint at the first type field...
			for (int i = 0; i < fields.size(); i++) {
				layout_field f = fields[i];

				JVL_ASSERT(!listed.contains(f.ptr),
					"duplicate member in uniform layout "
					"(field #{} conflicts with field #{})",
					i + 1, listed[f.ptr] + 1);

				thunder::Load load;
				load.src = up.id;
				load.idx = i;

				cache_index_t cit;
				cit = em.emit(load);

				f.ref_with(cit);

				listed[f.ptr] = i;
			}
		}
	}
};

inline void layout_field::ref_with(cache_index_t up)
{
	if (type == eNested) {
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
	const char *name;
	int type;
	const void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct const_uniform_layout_t {
	std::string name;
	std::vector <layout_const_field> fields;

	cache_index_t list() const;

	auto remove_const() const {
		uniform_layout_t <Args...> layout;
		layout.name = name;
		for (auto cf: fields) {
			// TODO: might crash with pointer to layouts...
			layout_field f;
			f.type = cf.type;
			f.ptr = const_cast <void *> (cf.ptr);
			layout.fields.push_back(f);
		}

		return layout;
	}
};

// Concept for ensuring that a struct can be synthesized
template <typename T>
concept uniform_compatible = requires(T &ref, const T &cref) {
	{
		cref.layout().fields
	} -> std::same_as <std::vector <layout_const_field> &&>;
};

// Named fields
template <string_literal name, typename T>
struct __field {
	static constexpr string_literal literal = name;
	const tagged *ref;
};

// Concept for fields
template <typename T>
struct __field_type_t : std::false_type {};

template <string_literal name, typename T>
struct __field_type_t <__field <name, T>> : std::true_type {};

template <typename T>
concept __field_type = __field_type_t <T> ::value;

// Backend methods for creating uniform layouts
template <synthesizable T, string_literal name, typename ... UArgs>
void __const_init(layout_const_field *fields, int index, const __field <name, T> &t, const UArgs &... uargs)
{
	layout_const_field f;
	f.name = name.value;
	f.type = eField;
	f.ptr = t.ref;

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

template <uniform_compatible T, string_literal name, typename ... UArgs>
void __const_init(layout_const_field *fields, int index, const __field <name, T> &t, const UArgs &... uargs)
{
	using layout_t = decltype(T().layout());

	layout_const_field f;
	f.name = name.value;
	f.type = eNested;
	f.ptr = new layout_t(t.layout());

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

template <string_literal_group field_group, typename ... Args>
struct higher_const_uniform_layout_t : const_uniform_layout_t <Args...> {
	static constexpr auto group = field_group;
};

template <uniform_compatible T, string_literal field>
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

#define uniform_field(T, name) uniform_field_index <T, #name> ()

} // namespace jvl::ire
