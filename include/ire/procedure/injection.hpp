#pragma once

#include "../parameters.hpp"

namespace jvl::ire {

// Generating type codes and filling references for parameters
template <typename T>
struct parameter_injection : std::false_type {};

template <builtin T>
struct parameter_injection <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void inject(T &x, thunder::Index i) {
		x.ref = i;
	}
};

template <aggregate T>
struct parameter_injection <T> : std::true_type {
	static thunder::Index emit() {
		auto v = T();
		return type_info_generator <T> (v)
			.synthesize()
			.concrete();
	}

	static void inject(T &x, thunder::Index i) {
		x.layout().link(i);
	}
};

template <generic T>
struct parameter_injection <in <T>> : std::true_type {
	using P = parameter_injection <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_in);
	}

	static void inject(in <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::inject(y, i);
	}
};

template <generic T>
struct parameter_injection <out <T>> : std::true_type {
	using P = parameter_injection <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_out);
	}

	static void inject(out <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::inject(y, i);
	}
};

template <generic T>
struct parameter_injection <inout <T>> : std::true_type {
	using P = parameter_injection <T>;

	static thunder::Index emit() {
		auto &em = Emitter::active;

		thunder::Index t = P::emit();
		return em.emit_qualifier(t, -1, thunder::qualifier_inout);
	}

	static void inject(inout <T> &x, thunder::Index i) {
		T &y = static_cast <T &> (x);
		P::inject(y, i);
	}
};

} // namespace jvl::ire