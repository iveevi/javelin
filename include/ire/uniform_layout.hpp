#pragma once

#include <array>

#include "../thunder/atom.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

enum {
	eField,
	eNested
};

// Ordinary layouts
struct layout_field {
	const char *name;
	int type;
	void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout_t {
	std::vector <layout_field> fields;

	// TODO: move down...
	void __ref_with(cache_index_t up) {
		std::unordered_set <void *> listed;

		auto &em = Emitter::active;

		// TODO: name hint at the first type field...
		for (size_t i = 0; i < fields.size(); i++) {
			layout_field f = fields[i];

			if (listed.contains(f.ptr))
				fmt::println("Duplicate member in uniform layout (element #{})", i + 1);

			thunder::Load load;
			load.src = up.id;
			load.idx = i;

			if (f.type == eNested) {
				using tmp_layout = uniform_layout_t <void>;

				cache_index_t cit;
				cit = em.emit(load);

				auto ul = reinterpret_cast <tmp_layout *> (f.ptr);
				ul->__ref_with(cit);
			} else {

				auto t = reinterpret_cast <tagged *> (f.ptr);
				t->ref = em.emit(load);
			}

			listed.insert(f.ptr);
		}
	}

	// TODO: name?
};

// Const-qualified layouts
struct layout_const_field {
	const char *name;
	int type;
	const void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct const_uniform_layout_t {
	std::vector <layout_const_field> fields;
	// TODO: name?

	cache_index_t list() const;

	auto remove_const() const {
		uniform_layout_t <Args...> layout;
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
template <size_t N>
struct string_literal {
	constexpr string_literal(const char (&str)[N]) {
		std::copy_n(str, N, value);
	}

	char value[N];
};

template <string_literal name, typename T>
struct __field {
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

} // namespace jvl::ire