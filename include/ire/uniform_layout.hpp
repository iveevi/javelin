#pragma once

#include <array>

#include "../atom/atom.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

enum {
	eField,
	eNested
};

// Ordinary layouts
struct field {
	int type;
	void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct __uniform_layout {
	std::vector <field> fields;

	// TODO: move down...
	void __ref_with(cache_index_t up) {
		std::unordered_set <void *> listed;

		auto &em = Emitter::active;

		// TODO: name hint at the first type field...
		for (size_t i = 0; i < fields.size(); i++) {
			field f = fields[i];

			if (listed.contains(f.ptr))
				fmt::println("Duplicate member in uniform layout (element #{})", i + 1);

			atom::Load load;
			load.src = up.id;
			load.idx = i;

			if (f.type == eNested) {
				using tmp_layout = __uniform_layout <void>;

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
struct const_field {
	int type;
	const void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct __const_uniform_layout {
	std::vector <const_field> fields;
	// TODO: name?

	cache_index_t list() const;

	auto remove_const() const {
		__uniform_layout <Args...> layout;
		for (auto cf: fields) {
			// TODO: might crash with pointer to layouts...
			field f;
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
	} -> std::same_as <std::vector <const_field> &&>;
};

// Backend methods for creating uniform layouts
template <synthesizable T, typename ... UArgs>
void __const_init(const_field *fields, int index, const T &t, const UArgs &... uargs)
{
	const_field f;
	f.type = eField;
	f.ptr = &static_cast <const tagged &> (t);

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

template <uniform_compatible T, typename ... UArgs>
void __const_init(const_field *fields, int index, const T &t, const UArgs &... uargs)
{
	using layout_t = decltype(T().layout());

	const_field f;
	f.type = eNested;
	f.ptr = new layout_t(t.layout());

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__const_init(fields, index + 1, uargs...);
}

// User end functions to specify members
template <typename ... Args>
auto uniform_layout(const Args &... args)
{
	__const_uniform_layout <Args...> layout;
	layout.fields.resize(sizeof...(Args));
	__const_init(layout.fields.data(), 0, args...);
	return layout;
}

} // namespace jvl::ire
