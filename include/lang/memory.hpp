#pragma once

#include <bitset>
#include <cstdlib>
#include <list>
#include <vector>

#include <fmt/printf.h>

#include "../wrapped_types.hpp"

namespace jvl::lang {

// Memory management
struct grand_bank_t {
	// Tracks memory usage from within the compiler
	wrapped::hash_table <void *, size_t> tracked;
	size_t bytes = 0;

	template <typename T>
	T *alloc(size_t elements) {
		size_t s = sizeof(T) * elements;
		bytes += s;
		T *ptr = (T *) std::malloc(s);
		// T *ptr = new T[elements];
		tracked[ptr] = s;
		return ptr;
	}

	template <typename T>
	void free(T *ptr) {
		if (!tracked.count(ptr)) {
			fmt::println("fatal error: {} not registered", (void *) ptr);
			abort();
		}

		if (tracked[ptr] > bytes) {
			fmt::println("fatal erorr: alloced pointer is registered for more than tracked");
			fmt::println("  grand bank currently holds {} bytes", bytes);
			fmt::println("  address to be freed ({}) tracked for {} bytes", (void *) ptr, tracked[ptr]);
			abort();
		}

		bytes -= tracked[ptr];
		tracked.erase(ptr);
		// delete[] ptr;
		std::free(ptr);
	}

	void dump() {
		fmt::println("Grand Bank:");
		fmt::println("  # of bytes remaining  {}", bytes);
		fmt::println("  # of items in track   {}", tracked.size());
	}

	static grand_bank_t &one() {
		static grand_bank_t gb;
		return gb;
	}
};

template <typename T>
constexpr size_t page_size_v = 16;

// Strings will be longer
template <>
constexpr size_t page_size_v <char> = 32;

template <typename T>
struct bank_t {
	static constexpr size_t page_size = page_size_v <T>;

	using page_mask_t = std::bitset <page_size>;

	struct page {
		T *addr = nullptr;
		page_mask_t used = 0;

		int find_slot(size_t elements) const {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++)
				packet.set(i);

			for (size_t i = 0; i < page_size - elements + 1; i++) {
				page_mask_t offset = packet << i;
				page_mask_t combined = offset & (~used);
				if (combined.count() == elements)
					return i;
			}

			return -1;
		}

		void free_slot(int slot, size_t elements) {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++) {
				addr[slot + i].~T();
				packet.set(i);
			}

			packet <<= slot;
			used &= (~packet);
		}

		void mark_used(int slot, size_t elements) {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++)
				packet.set(i);

			packet <<= slot;
			used |= packet;
		}

		size_t usage() const {
			return used.count();
		}

		void clear() {
			grand_bank_t::one().free(addr);
			addr = nullptr;
			used = 0;
		}

		static page alloc() {
			page p;
			p.addr = grand_bank_t::one().alloc <T> (page_size);
			p.used = 0;
			return p;
		}
	};

	using track_data_t = std::tuple <page *, int, size_t>;

	jvl::wrapped::hash_table <T *, track_data_t> tracked;

	std::list <page> page_list;

	std::tuple <page *, int> find_slot(size_t elements) {
		for (auto &p : page_list) {
			int slot = p.find_slot(elements);
			if (slot != -1)
				return { &p, slot };
		}

		return { nullptr, -1 };
	}

	T *alloc(size_t elements) {
		if (elements > page_size) {
			fmt::println("fatal error: too many elements requested");
			fmt::println("  requested {} elements", elements);
			fmt::println("  page size is {} elements", page_size);
			abort();
		}

		auto [p, slot] = find_slot(elements);
		bool insufficient = page_list.empty() || (slot == -1);

		if (insufficient)
			page_list.push_back(page::alloc());

		std::tie(p, slot) = find_slot(elements);
		if (slot == -1) {
			fmt::println("fatal error: failed to find slot");
			abort();
		}

		p->mark_used(slot, elements);

		T *ptr = &p->addr[slot];
		tracked[ptr] = std::make_tuple(p, slot, elements);

		return ptr;
	}

	void free(T *ptr) {
		if (!tracked.count(ptr)) {
			fmt::println("fatal error: {} not registered", (void *) ptr);
			abort();
		}

		auto [p, slot, elements] = tracked[ptr];

		// TODO: verify that p exists in debug mode

		p->free_slot(slot, elements);
		tracked.erase(ptr);

		// TODO: allow some dead time?
		// dispatch a thread which will do it if needed
		if (p->usage() == 0) {
			for (auto it = page_list.begin(); it != page_list.end(); it++) {
				if (&(*it) == p) {
					it->clear();
					page_list.erase(it);
					break;
				}
			}
		}
	}

	void clear() {
		for (auto page : page_list)
			page.clear();

		page_list.clear();
	}

	void dump() {
		fmt::println("Bank:");
		fmt::println("  # of pages: {}", page_list.size());

		size_t index = 0;
		for (auto page : page_list)
			fmt::println("  {:3d}: {}", index++, page.used);
	}

	static bank_t &one() {
		static bank_t b;
		return b;
	}
};

template <typename T>
struct ptr {
	T *address = nullptr;

	ptr(const std::nullptr_t &) : address(nullptr) {}
	ptr(T *address_) : address(address_) {}

	ptr &disowned() {
		address->disown();
		return *this;
	}

	operator bool() const {
		return address;
	}

	T &operator*() {
		return *address;
	}

	const T &operator*() const {
		return *address;
	}

	T *operator->() {
		return address;
	}

	const T *operator->() const {
		return address;
	}

	ptr &operator++(int) {
		address++;
		return *this;
	}

	void free() {
		if (address)
			bank_t <T> ::one().free(address);
		address = nullptr;
	}

	template <typename ... Args>
	static ptr from(const Args &... args) {
		T *p = bank_t <T> ::one().alloc(1);
		new (p) T(args...);
		return p;
	}

	static ptr block_from(std::vector <ptr <T>> &values) {
		T *p = bank_t <T> ::one().alloc(values.size());
		for (size_t i = 0; i < values.size(); i++)
			new (&p[i]) T(std::move(*values[i]));
		return p;
	}
};

} // namespace jvl::lang