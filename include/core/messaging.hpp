#pragma once

#include <cstdint>
#include <queue>

#include "wrapped_types.hpp"
#include "math_types.hpp"

namespace jvl::core {

struct UUID {
	int64_t global;
	int64_t type_id;
	int64_t type_local;
};

struct Unique {
	core::UUID uuid;
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
	editor_update_selected,
};

struct Message {
	int64_t type_id;
	int64_t global;

	MessageKind kind;

	wrapped::variant <
		int64_t,
		int32_t, int2,
		float, float2
	> value;
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

} // namespace jvl::core