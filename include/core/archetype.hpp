#pragma once

#include "messaging.hpp"

namespace jvl::core {

// Vectorized archetype of mark entities
template <marked T>
class Archetype {
	std::vector <T> data;
	std::map <int64_t, int64_t> global_to_index;
public:
	// Reference to an archetype element
	class Reference {
		// The location of the vector itself should not change...
		std::vector <T> *data;
		int64_t index;

		Reference(std::vector <T> *data_, int64_t index_)
			: data(data_), index(index_) {}
	public:
		// Access
		T &operator*() {
			return (*data)[index];
		}
		
		T *operator->() {
			return &(*data)[index];
		}

		friend class Archetype;
	};
	
	// Read-only reference to an archetype element
	class ConstantReference {
		// The location of the vector itself should not change...
		const std::vector <T> *data;
		int64_t index;
	public:
		// Access
		const T &operator*() {
			return (*data)[index];
		}
		
		const T *operator->() {
			return &(*data)[index];
		}

		friend class Archetype;
	};

	Archetype() = default;

	Archetype(const Archetype &) = delete;
	Archetype operator=(const Archetype &) = delete;

	Archetype &add(const T &value) {
		int64_t index = data.size();
		data.push_back(value);

		int64_t global = data.back().id();
		global_to_index[global] = index;
		fmt::println("adding element ({}, {})", global, index);
		return *this;
	}

	Reference operator[](int64_t index) {
		return { &data, index };
	}

	ConstantReference operator[](int64_t index) const {
		return { &data, index };
	}
	
	Reference mapped(int64_t global) {
		int64_t index = global_to_index.at(global);
		return { &data, index };
	}

	ConstantReference mapped(int64_t global) const {
		int64_t index = global_to_index.at(global);
		return { &data, index };
	}

	size_t size() const {
		return data.size();
	}
};

} // namespace jvl::core