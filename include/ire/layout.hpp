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

template <bool, field_type ... Fields>
struct Layout;

template <typename T>
struct is_layout_type : std::false_type {};

template <bool Phantom, field_type ... Fields>
struct is_layout_type <Layout <Phantom, Fields...>> : std::true_type {};

template <typename T>
concept layout_type = is_layout_type <T> ::value;

// TODO: ensure it has a default constructor...
template <typename T>
concept aggregate = requires(T &t) {
	{ t.layout() } -> layout_type;
};

// Unique type index generator
inline int64_t type_index_counter = 0;

template <typename T>
int64_t type_counter()
{
	// Force ID fetch at runtime
	static int64_t c = -1;
	if (c == -1)
		c = type_index_counter++;
	return c;
}

template <bool Phantom, field_type ... Fields>
struct Layout {
	using T = aggregate_wrapper <typename Fields::underlying...>;

	std::string name;
	std::tuple <Fields...> references;

	Layout(const std::string &name_, const Fields &... args) : name(name_), references(args...) {}

	void add_decorations(thunder::Index src, bool type) {
		auto &em = Emitter::active;

		fmt::println("HINTING FOR {}", src);

		// TODO: evaluate once statically?
		std::vector <std::string> fields = std::apply([](const auto &... fields) {
			return std::vector <std::string> { fields.name... };
		}, references);

		em.add_type_hint(src, type_counter <T> (), name, fields);
		if (Phantom && type)
			em.add_phantom_hint(src);
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
			add_decorations(src, false);
	}

	template <size_t I = 0>
	thunder::Index reconstruct() {
		auto &em = Emitter::active;
		auto &ref = std::get <I> (references).ref;

		using E = decltype(ref);

		static_assert(builtin <E>, "reconstruct not implemented for nested structs");

		thunder::List list;
		if constexpr (builtin <E>)
			list.item = ref.synthesize().id;

		if constexpr (I + 1 < sizeof...(Fields))
			list.next = reconstruct <I + 1> ();

		return em.emit(list);
	}
	
	intermediate_type generate_type() {	
		intermediate_type type = std::apply([](const auto &... fields) {
			return type_info_generator <T> (fields.ref...).synthesize();
		}, references);

		add_decorations(type.as <composite_type> ().idx, true);

		return type;
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
	return Layout <false, Fields...> (s, fields...);
}

template <field_type ... Fields>
auto phantom_layout_from(const std::string &s, const Fields &... fields)
{
	return Layout <true, Fields...> (s, fields...);
}

#define verbatim_field(name) jvl::ire::field <decltype(name)> (#name, name)

} // namespace jvl::ire