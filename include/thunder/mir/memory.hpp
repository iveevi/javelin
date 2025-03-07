#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace jvl::thunder::mir {

/////////////////////////////////////////////////////////////////
// Memory management for Molecular intermediate representation //
/////////////////////////////////////////////////////////////////

// Index wrapper for type checking
struct Index {
	using index_type = int32_t;

	index_type value;

	Index() = default;
	Index(index_type value) : value(value) {}

	operator index_type() const {
		return value;
	}

	Index &operator++(int) {
		value++;
		return *this;
	}
};

// Allocator for a list of elements
template <typename Base>
struct Allocator {
	std::vector <Base> data;

	Allocator() = default;

	template <typename T, typename... Args>
	Index allocate_one(Args &&... args) {
		Index index = data.size();
		data.emplace_back(T(std::forward <Args> (args)...));
		return index;
	}
	
	Index allocate_many(size_t count) {
		Index index = data.size();
		data.resize(data.size() + count, Base());
		return index;
	}

	template <typename T>
	Index allocate_many(const std::vector <T> &elements) {
		Index index = data.size();
		data.insert(data.end(), elements.begin(), elements.end());
		return index;
	}

	template <typename T>
	T *get(Index index) {
		if constexpr (std::same_as <T, Base>)
			return &data[index];
		else
			return &data[index].template as <T> ();
	}

	Base *get_base(Index index) {
		return &data[index];
	}

	void copy(Index dst, Index src) {
		data[dst] = data[src];
	}

	static Allocator active;
};

template <typename Base>
Allocator <Base> Allocator <Base> ::active;

// Single pointer to an element
template <typename T, typename Base>
struct Ptr {
	using TAllocator = Allocator <Base>;

	Index index;

	Ptr() : index(-1) {}

	Ptr(const T &v) {
		index = TAllocator::active.template allocate_one <T> (v);
	}

	Ptr(const Index &index) : index(index) {}

	Ptr(const Ptr &other) {
		index = other.index;
	}

	// Accessor methods
	T &operator*() {
		return *TAllocator::active.template get <T> (index);
	}

	const T &operator*() const {
		return *TAllocator::active.template get <T> (index);
	}

	T *operator->() {
		return TAllocator::active.template get <T> (index);
	}

	const T *operator->() const {
		return TAllocator::active.template get <T> (index);
	}

	Index::index_type idx() const {
		return index.value;
	}

	// Promote to base type
	Ptr <Base, Base> promote() const {
		return index;
	}

	// Status
	operator bool() const {
		return index != -1;
	}
};

// Contiguous list of elements
template <typename T, typename Base>
struct List {
	using TAllocator = Allocator <Base>;

	Index head;
	uint32_t count;
	uint32_t capacity;

	List() : head(-1), count(0), capacity(0) {}

	List(const std::vector <T> &elements) {
		head = TAllocator::active.template allocate_many <T> (elements);
		count = elements.size();
		capacity = count;
	}

	// Modifiers
	void add(const T &element) {
		// TODO: free list management...
		if (count + 1 > capacity) {
			capacity = std::max(2 * capacity + 1, 4u);

			auto index = TAllocator::active.allocate_many(capacity);
			
			// Copy the old list
			for (uint32_t i = 0; i < count; i++)
				TAllocator::active.copy(index + i, head + i);

			// Add the new element
			*TAllocator::active.get_base(index + count) = element;

			head = index;
		} else {
			*TAllocator::active.get_base(head + count) = element;
		}
	
		// Size should always increase
		count++;
	}

	// Mutable iterators
	struct iterator {
		Index index;

		iterator(Index index) : index(index) {}

		iterator &operator++() {
			index++;
			return *this;
		}

		T &operator*() {
			return *TAllocator::active.template get <T> (index);
		}

		bool operator!=(const iterator &other) const {
			return index != other.index;
		}
	};

	iterator begin() {
		return iterator(head);
	}

	iterator end() {
		return iterator(head + count);
	}

	// Immutable iterators
	struct const_iterator {
		Index index;

		const_iterator(Index index) : index(index) {}

		const_iterator &operator++() {
			index++;
			return *this;
		}

		const T &operator*() {
			return *TAllocator::active.template get <T> (index);
		}

		bool operator!=(const const_iterator &other) const {
			return index != other.index;
		}
	};

	const_iterator begin() const {
		return const_iterator(head);
	}

	const_iterator end() const {
		return const_iterator(head + count);
	}
};

} // namespace jvl::thunder::mir