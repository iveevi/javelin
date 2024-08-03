#pragma once

#include <optional>
#include <unordered_map>
#include <variant>

// Wrappers on standard types
namespace jvl::wrapped {

template <typename... Args>
struct variant : std::variant<Args...> {
	using std::variant<Args...>::variant;

	template <typename T>
	inline bool is()
	{
		return std::holds_alternative<T>(*this);
	}

	template <typename T>
	inline auto as()
	{
		return std::get<T>(*this);
	}

	template <typename T>
	inline auto as() const
	{
		return std::get<T>(*this);
	}
};

template <typename T>
struct optional : std::optional<T> {
	using std::optional<T>::optional;
};

template <typename... Args>
struct optional<variant<Args...>> : std::optional<variant<Args...>> {
	using std::optional<variant<Args...>>::optional;

	template <typename T>
	inline optional<T> as()
	{
		if (this->has_value())
			return (this->value()).template as<T>();

		return std::nullopt;
	}
};

template <typename K, typename V>
struct hash_table : std::unordered_map<K, V> {
	using std::unordered_map<K, V>::unordered_map;

	optional<V> maybe_at(const K &k)
	{
		if (this->count(k))
			return this->operator[](k);

		return std::nullopt;
	}

	optional<V> maybe_at(const K &k) const
	{
		if (this->count(k))
			return this->at(k);

		return std::nullopt;
	}
};

} // namespace jvl::wrapped
