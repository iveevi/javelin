#pragma once

#include "solid.hpp"

namespace jvl::ire {

// Underlying interface for soft_t
template <typename Element, field_type ... Fields>
class fragment_zipper : public std::vector <uint8_t> {
	using tuple = typename solid_padded_builder <Layout <false, Fields...>> ::type;

	static constexpr size_t tuple_size() {
		// TODO: padding if necessary...
		return sizeof(tuple);
	}

	// Accessing offset data
	size_t elements_size;

	Element *dptr(size_t offset) {
		uint8_t *ptr = data() + tuple_size();
		return reinterpret_cast <Element *> (ptr) + offset;
	}
	
	Element *const dptr(size_t offset) const {
		uint8_t *const ptr = data() + tuple_size();
		return reinterpret_cast <Element *const> (ptr) + offset;
	}
	
	Element &dref(size_t offset) {
		return *dptr(offset);
	}
	
	const Element &dref(size_t offset) const {
		return *dptr(offset);
	}

	// Extending dynamic memory at the end
	void extend(size_t elements) {
		size_t new_size = size() + elements * sizeof(Element);
		resize(new_size);
	}
public:
	fragment_zipper() : std::vector <uint8_t> (tuple_size(), 0), elements_size(0) {}

	// Tuple methods at the front of the memory
	template <size_t I>
	requires (0 <= I && I <= sizeof...(Fields))
	auto &get() {
		return ((tuple *) data())->template get <I> ();
	}
	
	template <size_t I>
	requires (0 <= I && I <= sizeof...(Fields))
	const auto &get() const {
		return ((tuple *const) data())->template get <I> ();
	}

	// Vector methods at the end of the memory
	size_t count() {
		return elements_size;
	}

	void push(const Element &element) {
		extend(1);
		dref(elements_size++) = element;
	}

	void extend(const std::vector <Element> &elements) {
		extend(elements.size());
		std::memcpy(dptr(elements_size), elements.data(), sizeof(Element) * elements.size());
		elements_size += elements.size();
	}

	Element &operator[](size_t idx) {
		return dref(idx);
	}

	const Element &operator[](size_t idx) const {
		return dref(idx);
	}
};

} // namespace jvl::ire