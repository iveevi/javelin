#pragma once

#include <type_traits>
#include <algorithm>

#include "type.hpp"
#include "emitter.hpp"
#include "tagged.hpp"

namespace jvl::ire {

template <size_t N>
struct string_literal {
	constexpr string_literal(const char (&str)[N]) {
		std::copy_n(str, N, value);
	}

	char value[N];
};

template <typename T>
struct field {
	using underlying = std::decay_t <T>;

	std::string name;
	T &ref;

	field(const std::string &name_, T &val) : name(name_), ref(val) {}
};

template <typename T>
struct is_field_type : std::false_type {};

template <typename T>
struct is_field_type <field <T>> : std::true_type {};

template <typename T>
concept field_type = is_field_type <T> ::value;

template <field_type ... Fields>
struct Layout;

template <typename T>
struct is_layout_type : std::false_type {};

template <field_type ... Fields>
struct is_layout_type <Layout <Fields...>> : std::true_type {};

template <typename T>
concept layout_type = is_layout_type <T> ::value;

template <typename T>
concept aggregate = requires(T &t) {
	{ t.layout() } -> layout_type;
};

// Unique type index generator
inline int64_t type_index_counter = 0;

template <typename T>
int64_t type_index()
{
	// Force ID fetch at runtime
	static int64_t c = -1;
	if (c == -1)
		c = type_index_counter++;
	return c;
}

template <field_type ... Fields>
struct Layout {
	using T = aggregate_wrapper <typename Fields::underlying...>;

	std::string name;
	std::tuple <Fields...> references;

	Layout(const std::string &name_, const Fields &... args) : name(name_), references(args...) {}

	void embed_hint(thunder::Index src) {
		auto &em = Emitter::active;

		// TODO: evaluate once statically?
		std::vector <std::string> fields = std::apply([](const auto &... fields) {
			return std::vector <std::string> { fields.name... };
		}, references);

		em.emit_hint(src, type_index <T> (), name, fields);
	}

	template <size_t I = 0>
	void link(thunder::Index src) {
		auto &em = Emitter::active;
		auto &ref = std::get <I> (references).ref;

		using E = decltype(ref);

		// TODO: aggregates
		auto idx = em.emit_load(src, thunder::Index(I));
		if constexpr (builtin <E>)
			ref.ref = cache_index_t::from(idx);
		else if constexpr (aggregate <E>)
			ref.layout().link(idx);

		if constexpr (I + 1 < sizeof...(Fields))
			link <I + 1> (src);

		if constexpr (I == 0)
			embed_hint(src);
	}
	
	intermediate_type generate_type() {	
		auto &em = Emitter::active;

		intermediate_type idx = std::apply([](const auto &... fields) {
			return type_info_generator <T> (fields.ref...).synthesize();
		}, references);

		embed_hint(idx.as <struct_type> ().idx);

		return idx;
	}
};

// Override type generation for aggregates
template <aggregate T>
struct type_info_generator <T> {
	T &ref;

	intermediate_type synthesize() {
		return ref.layout().generate_type();
	}
};

// Easier construction
template <field_type ... Fields>
auto layout_from(const std::string &s, const Fields &... fields)
{
	return Layout <Fields...> (s, fields...);
}

#define verbatim_field(name) field <decltype(name)> (#name, name)

} // namespace jvl::ire