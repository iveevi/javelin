#pragma once

#include "../parameters.hpp"

namespace jvl::ire {

// Generating type codes and filling references for parameters
template <typename T>
struct parameter_injector : std::false_type {};

template <builtin T>
struct parameter_injector <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void apply(T &x, thunder::Index i) {
		x.ref = i;
	}
};

template <aggregate T>
struct parameter_injector <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void apply(T &x, thunder::Index i) {
		x.layout().link(i);
	}
};

template <generic T>
struct parameter_injector <in <T>> : std::true_type {
	using P = parameter_injector <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_in);
	}

	static void apply(in <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::apply(y, i);
	}
};

template <generic T>
struct parameter_injector <out <T>> : std::true_type {
	using P = parameter_injector <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_out);
	}

	static void apply(out <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::apply(y, i);
	}
};

template <generic T>
struct parameter_injector <inout <T>> : std::true_type {
	using P = parameter_injector <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_inout);
	}

	static void apply(inout <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::apply(y, i);
	}
};

} // namespace jvl::ire