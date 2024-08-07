#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

// Wrappers on standard types
namespace jvl::wrapped {

// Easier to work with variants
template <typename ... Args>
struct variant : std::variant <Args...> {
	using std::variant <Args...> ::variant;

	template <typename T>
	inline bool is() const {
		return std::holds_alternative <T> (*this);
	}

	template <typename T>
	inline auto &as() {
		return std::get <T> (*this);
	}

	template <typename T>
	inline const auto &as() const {
		return std::get <T> (*this);
	}

	template <typename T>
	inline std::optional <T> get() const {
		if (std::holds_alternative <T> (*this))
			return std::get <T> (*this);

		return std::nullopt;
	}
};

// Merging optional variant types
template <typename T>
struct optional : std::optional <T> {
	using std::optional <T> ::optional;
};

template <typename ... Args>
struct optional <variant <Args...>> : std::optional <variant <Args...>> {
	using std::optional <variant <Args...>> ::optional;

	template <typename T>
	inline optional <T> as() {
		if (this->has_value())
			return (this->value()).template as<T>();

		return std::nullopt;
	}
};

// Optional returns for hash tables
template <typename K, typename V>
struct hash_table : std::unordered_map <K, V> {
	using std::unordered_map <K, V> ::unordered_map;

	optional <V> maybe_at(const K &k) {
		if (this->count(k))
			return this->operator[](k);

		return std::nullopt;
	}

	optional <V> maybe_at(const K &k) const {
		if (this->count(k))
			return this->at(k);

		return std::nullopt;
	}
};

// Reindexing map
struct reindex : std::unordered_map <int, int> {
	reindex() : std::unordered_map <int, int> { { -1, -1 } } {}

	int operator()(int i) {
		if (contains(i))
			return operator[](i);

		return -1;
	}

	std::unordered_set <int> operator()(std::unordered_set <int> &set) {
		std::unordered_set <int> replace;
		for (int i : set)
			replace.insert(operator()(i));

		return replace;
	}
};

// Thread safe queue for work items
template <typename T>
struct thread_safe_queue : private std::deque <T> {
	std::mutex lock;

	thread_safe_queue() {}

	thread_safe_queue(const thread_safe_queue &tsq)
			: std::deque <T> (tsq) {}

	void push(const T &v) {
		std::lock_guard guard(lock);

		this->push_back(v);
	}

	void push(const thread_safe_queue <T> &tsq) {
		std::lock_guard guard(lock);

		for (auto &v : tsq)
			this->push_back(v);
	}

	T pop() {
		std::lock_guard guard(lock);

		T v = this->front();
		this->pop_front();
		return v;
	}

	T pop_locked() {
		T v = this->front();
		this->pop_front();
		return v;
	}

	[[gnu::always_inline]]
	size_t size() const {
		std::lock_guard guard(lock);
		return std::deque <T> ::size();
	}

	[[gnu::always_inline]]
	size_t size_locked() const {
		return std::deque <T> ::size();
	}

	[[gnu::always_inline]]
	void clear() {
		std::lock_guard guard(lock);
		return std::deque <T> ::clear();
	}
};

} // namespace jvl::wrapped
