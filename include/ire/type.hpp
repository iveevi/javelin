#pragma once

#include "../thunder/atom.hpp"
#include "../thunder/enumerations.hpp"
#include "emitter.hpp"

namespace jvl::ire {

struct primitive_type {
	thunder::PrimitiveType ptv;
};

struct composite_type {
	thunder::Index idx;
};

// TODO: method concrete() -> Index
using intermediate_type_base = bestd::variant <primitive_type, composite_type>;

struct intermediate_type : intermediate_type_base {
	using intermediate_type_base::intermediate_type_base;

	thunder::Index concrete() const {
		auto &em = Emitter::active;

		if (is <primitive_type> ())
			return em.emit_type_information(-1, -1, as <primitive_type> ().ptv);

		return as <composite_type> ().idx;
	}
};

// Top-level type generation
template <typename T>
struct type_info_generator {
	static_assert(false, "no type generator override for given type");
};

// General form is explicitly a struct with fields of type Args...
template <typename ... Args>
struct aggregate_wrapper {};

template <typename ... Args>
struct type_info_generator <aggregate_wrapper <Args...>> {
	static constexpr size_t count = sizeof...(Args);

	using tuple_type = std::tuple <Args...>;

	template <size_t I>
	using tuple_element = std::tuple_element_t <I, tuple_type>;

	tuple_type args;

	type_info_generator(const Args &... args_) : args(args_...) {}

	template <size_t I>
	intermediate_type partial() {
		auto &em = Emitter::active;

		intermediate_type current = type_info_generator <tuple_element <I>> (std::get <I> (args)).synthesize();

		if constexpr (I + 1 >= count) {
			auto end = em.emit_type_information(-1, -1, thunder::nil);

			auto &buffer = em.scopes.top().get();
			fmt::println("nil as uint64_t: {:016x}", reinterpret_cast <uint64_t &> (buffer.atoms[end]));
			
			auto end2 = em.emit_type_information(-1, -1, thunder::nil);
			fmt::println("---- nil as uint64_t: {:016x}", reinterpret_cast <uint64_t &> (buffer.atoms[end2]));

			if (current.is <primitive_type> ()) {
				auto &p = current.as <primitive_type> ();
				auto idx = em.emit_type_information(-1, end, p.ptv);
				return composite_type(idx);
			} else {
				auto &s = current.as <composite_type> ();
				auto idx = em.emit_type_information(s.idx, end, thunder::bad);
				return composite_type(idx);
			}
		} else {
			intermediate_type previous = partial <I + 1> ();

			auto &sp = previous.as <composite_type> ();
			
			if (current.is <primitive_type> ()) {
				auto &p = current.as <primitive_type> ();
				auto idx = em.emit_type_information(-1, sp.idx, p.ptv);
				return composite_type(idx);
			} else {
				auto &s = current.as <composite_type> ();
				auto idx = em.emit_type_information(s.idx, sp.idx, thunder::bad);
				return composite_type(idx);
			}
		}
	}

	// TODO: optionally pass type hints...
	intermediate_type synthesize() {
		return partial <0> ();
	}
};

} // namespace jvl::ire