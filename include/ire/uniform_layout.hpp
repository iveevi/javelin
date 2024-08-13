#pragma once

#include <array>

#include "../atom/atom.hpp"
#include "tagged.hpp"
#include "emitter.hpp"

namespace jvl::ire {

struct field {
	enum {
		eField,
		eNested
	} type;

	void *ptr;
};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct __uniform_layout {
	std::vector <field> fields;

	void __ref_with(cache_index_t up) {
		auto &em = Emitter::active;

		for (size_t i = 0; i < fields.size(); i++) {
			field f = fields[i];

			atom::Load load;
			load.src = up.id;
			load.idx = i;

			if (f.type == field::eNested) {
				using tmp_layout = __uniform_layout <void>;

				cache_index_t cit;
				cit = em.emit(load);

				auto ul = reinterpret_cast <tmp_layout *> (f.ptr);
				ul->__ref_with(cit);
			} else {

				auto t = reinterpret_cast <tagged *> (f.ptr);
				t->ref = em.emit(load);
			}
		}
	}

	// TODO: name?
};

template <typename T>
struct is_uniform_layout {
	static constexpr bool value = false;
};

template <typename ... Args>
struct is_uniform_layout <__uniform_layout <Args...>> {
	static constexpr bool value = true;
};

template <typename T>
concept uniform_compatible = is_uniform_layout <decltype(T().layout())> ::value;

template <synthesizable T, typename ... UArgs>
void __init(field *fields, int index, T &t, UArgs &... uargs)
{
	field f;
	f.type = field::eField;
	f.ptr = &static_cast <tagged &> (t);

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__init(fields, index + 1, uargs...);
}

template <uniform_compatible T, typename ... UArgs>
void __init(field *fields, int index, T &t, UArgs &... uargs)
{
	using layout_t = decltype(T().layout());

	field f;
	f.type = field::eNested;
	f.ptr = new layout_t(t.layout());

	fields[index] = f;
	if constexpr (sizeof...(uargs) > 0)
		__init(fields, index + 1, uargs...);
}

template <typename ... Args>
auto uniform_layout(Args &... args)
{
	__uniform_layout <Args...> layout;
	layout.fields.resize(sizeof...(Args));
	__init(layout.fields.data(), 0, args...);
	return layout;
}

} // namespace jvl::ire
