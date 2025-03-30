#pragma once

#include <type_traits>

#include "../../thunder/tracked_buffer.hpp"
#include "../control_flow.hpp"
#include "../tagged.hpp"
#include "injection.hpp"

namespace jvl::ire {

// Internal construction of procedures
template <generic_or_void R, typename ... Args>
struct Procedure : thunder::TrackedBuffer {
	// TODO: put outside this defn
	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl)
	requires (sizeof...(Args) > 0) {
		auto &em = Emitter::active;

		using T = std::decay_t <decltype(std::get <index> (tpl))>;
		using P = parameter_injection <T>;

		auto &x = std::get <index> (tpl);

		// Parameters in the target language
		if constexpr (P::value) {
			thunder::Index t = P::emit();
			thunder::Index q = em.emit_qualifier(t, index, thunder::parameter);
			thunder::Index c = em.emit_construct(q, -1, thunder::global);
			P::inject(x, c);
		}

		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	void begin() {
		Emitter::active.push(*this);
	}

	void call(std::tuple <Args...> &args) {
		if constexpr (sizeof...(Args))
			__fill_parameter_references <0> (args);
	}

	void end() {
		Emitter::active.pop();
	}

	auto &named(const std::string &name_) {
		name = name_;
		return *this;
	}

	R operator()(Args ... args) {
		auto &em = Emitter::active;

		// If R is an aggregate, we need to bind its members
		if constexpr (aggregate <R>) {
			R instance;

			auto layout = instance.layout().remove_const();

			thunder::Call call;
			call.cid = cid;
			call.type = type_field_from_args(layout).id;
			call.args = list_from_args(args...);

			cache_index_t cit;
			cit = em.emit(call);

			layout.ref_with(cit);
			return instance;
		}

		// For primitives
		thunder::Call call;

		auto r = R();

		call.cid = cid;
		call.args = list_from_args(args...);
		call.type = type_info_generator <R> (r)
			.synthesize()
			.concrete();

		cache_index_t cit;
		cit = em.emit(call);

		return R(cit);
	}
};

} // namespace jvl::ire
