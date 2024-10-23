#pragma once

#include <cstdint>
#include <map>
#include <queue>

#include "wrapped_types.hpp"
#include "math_types.hpp"

#include <fmt/printf.h>

namespace jvl::core {

struct UUID {
	int64_t global;
	int64_t type_id;
	int64_t type_local;
};

// ID generation for each type
[[gnu::always_inline]]
inline int64_t new_type_id()
{
	static int64_t uuid = 0;
	return uuid++;
}

template <typename T>
[[gnu::always_inline]]
inline int64_t type_id_of()
{
	static int64_t id = new_type_id();
	return id;
}

// Global object ID
[[gnu::always_inline]]
inline int64_t new_global_uuid()
{
	static int64_t uuid = 0;
	return uuid++;
}

// Per-type global object ID
template <typename T>
[[gnu::always_inline]]
inline int64_t new_type_local_uuid()
{
	static int64_t uuid = 0;
	return uuid++;
}

template <typename T>
[[gnu::always_inline]]
inline UUID new_uuid()
{
	return UUID {
		.global = new_global_uuid(),
		.type_id = type_id_of <T> (),
		.type_local = new_type_local_uuid <T> (),
	};
}

// UUID based messaging system
enum MessageKind : int64_t {
	editor_remove_self,
	editor_viewport_selection,
	editor_viewport_update_selected,
	editor_update_selected_object,
	editor_open_material_inspector,
};

using MessageValue = wrapped::variant <
	int64_t,
	int32_t, int2,
	float, float2
>;

struct Message {
	int64_t global;
	int64_t type_id;
	MessageKind kind;
	MessageValue value;
};

class MessageSystem {
	using message_queue = std::queue <Message>;

	wrapped::hash_table <int64_t, message_queue> queues;
public:
	void send_to_origin(const Message &msg) {
		queues[-1].push(msg);
	}

	message_queue &get(int64_t global_uuid) {
		return queues[global_uuid];
	}

	message_queue &get_origin() {
		return queues[-1];
	}
};

// Easier entrace into the UUID system
struct Unique {
	UUID uuid;

	// Require that the ID be initialized
	Unique(const UUID &uuid_) : uuid(uuid_) {}

	// Global ID
	int64_t id() const {
		return uuid.global;
	}

	// Message template
	Message message(MessageKind kind, MessageValue value = 0) const {
		return Message(uuid.global, uuid.type_id, kind, value);
	}
};

template <typename T>
concept marked = std::is_base_of_v <Unique, T>;

// Vectorized archetype of mark entities
template <marked T>
class Archetype {
	std::vector <T> data;
	std::map <int64_t, int64_t> global_to_index;
public:
	Archetype &add(const T &value) {
		int64_t index = data.size();
		data.push_back(value);

		int64_t global = data.back().id();
		global_to_index[global] = index;
		fmt::println("adding element ({}, {})", global, index);
		return *this;
	}

	T &operator[](size_t index) {
		return data[index];
	}

	const T &operator[](size_t index) const {
		return data[index];
	}
	
	T &mapped(int64_t global) {
		int64_t index = global_to_index.at(global);
		return this->operator[](index);
	}

	const T &mapped(int64_t global) const {
		int64_t index = global_to_index.at(global);
		return this->operator[](index);
	}

	size_t size() const {
		return data.size();
	}
};

// Tracking UUID relationships
template <marked A, marked B>
struct Equivalence {
	std::map <int64_t, int64_t> a_to_b;
	std::map <int64_t, int64_t> b_to_a;

	const int64_t &operator[](const A &a) const {
		return a_to_b.at(a.id());
	}

	// TODO: strictly operate on the type...
	void add(const A &a, const B &b) {
		a_to_b[a.id()] = b.id();
		b_to_a[b.id()] = a.id();
	}
	
	void add(const A &a, int64_t b_id) {
		a_to_b[a.id()] = b_id;
		b_to_a[b_id] = a.id();
	}
	
	void add(int64_t aid, const B &b) {
		a_to_b[aid] = b.id();
		b_to_a[b.id()] = aid;
	}

	// TODO: strictly operate on the type...
	bool has_a(int64_t aid) const {
		return a_to_b.contains(aid);
	}

	// TODO: strictly operate on the type...
	bool has_b(int64_t bid) const {
		return b_to_a.contains(bid);
	}
};

} // namespace jvl::core