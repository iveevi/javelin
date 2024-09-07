#pragma once

#include <type_traits>

#include "../thunder/atom.hpp"
#include "../thunder/kernel.hpp"
#include "../thunder/scratch.hpp"
#include "uniform_layout.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

struct Callable : thunder::Scratch {
	// Global list of callables
	static auto &tracked() {
		static std::unordered_map <size_t, Callable *> map;
		return map;
	}

	static Callable *search_tracked(size_t cid) {
		auto &t = tracked();
		if (t.contains(cid))
			return t[cid];

		return nullptr;
	}

	// Unique id
	size_t cid;

	// An optional name (defaults to "callable<cid>")
	std::string name;

	// For callables we can track back used and synthesized
	// insructions from working backwards at the returns

	Callable();
	Callable(const Callable &);
	Callable &operator=(const Callable &);

	// TODO: destructor, which offloads it from the global list

	thunder::Kernel export_to_kernel() const;

	void dump();
};

// Internal construction of callables
template <non_trivial_generic_or_void R, generic ... Args>
struct callable_t : Callable {
	// TODO: only R needs to be restricted, the rest can be filtered depending on synthesizable or not...
	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl) {
		auto &em = Emitter::active;

		using type_t = std::decay_t <decltype(std::get <index> (tpl))>;

		if constexpr (uniform_compatible <type_t>) {
			auto &x = std::get <index> (tpl);

			auto layout = x.layout().remove_const();

			thunder::Global global;
			global.qualifier = thunder::GlobalQualifier::parameter;
			global.type = type_field_from_args(layout).id;
			global.binding = index;

			cache_index_t ref;
			ref = em.emit(global);
			layout.__ref_with(ref);
		} else {
			auto &x = std::get <index> (tpl);

			using type_of_x = std::decay_t <decltype(x)>;

			thunder::Global global;
			global.qualifier = thunder::GlobalQualifier::parameter;
			global.type = type_field_from_args <type_of_x> ().id;
			global.binding = index;

			x.ref = em.emit(global);
		}

		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	void begin() {
		Emitter::active.scopes.push(*this);
	}

	void call(std::tuple <Args...> &args) {
		__fill_parameter_references <0> (args);
	}

	void end() {
		Emitter::active.scopes.pop();
	}

	auto &named(const std::string &name_) {
		name = name_;
		return *this;
	}

	R operator()(const Args &... args) {
		auto &em = Emitter::active;

		// If R is uniform compatible, then we need to bind its members...
		if constexpr (uniform_compatible <R>) {
			R instance;

			auto layout = instance.layout().remove_const();

			thunder::Call call;
			call.cid = cid;
			call.type = type_field_from_args(layout).id;
			call.args = list_from_args(args...);

			cache_index_t cit;
			cit = em.emit(call);

			layout.__ref_with(cit);
			return instance;
		}

		// For primitives
		thunder::Call call;
		call.cid = cid;
		call.type = type_field_from_args <R> ().id;
		call.args = list_from_args(args...);

		cache_index_t cit;
		cit = em.emit(call);
		return R(cit);
	}
};

// Metaprogramming to generate the correct callable_t for native function types
namespace detail {

// TODO: find another base than std function
template <typename F>
concept acceptable_callable = std::is_function_v <F> || requires(const F &ftn) {
	{ std::function(ftn) };
};

template <non_trivial_generic_or_void R, generic ... Args>
struct signature_pair {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using callable = callable_t <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_callable = callable_t <RR, std::decay_t <Args>...>;
};

template <typename R, typename ... Args>
signature_pair <R, Args...> function_signature_cast(std::function <R (Args...)>)
{
	return {};
}

template <acceptable_callable F>
auto function_signature(const F &ftn)
{
	return function_signature_cast(std::function(ftn));
}

template <acceptable_callable F>
struct signature {
	using pair = decltype(function_signature(F()));

	using return_t = pair::return_t;
	using args_t = pair::args_t;

	using callable = pair::callable;

	template <typename RR>
	using manual_callable = typename pair::template manual_callable <RR>;
};

// Exception for real functions, which cannot be instantiated
template <non_trivial_generic R, generic ... Args>
struct signature <R (Args...)> {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using callable = callable_t <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_callable = callable_t <RR, std::decay_t <Args>...>;
};

}

// For functions which have concrete return types already
// TODO: type restrictions with concepts
template <detail::acceptable_callable F>
requires (!std::same_as <typename detail::signature <F> ::return_t, void>)
auto callable(F ftn)
{
	using S = detail::signature <F>;

	typename S::callable cbl;

	cbl.begin();
		auto args = typename S::args_t();
		cbl.call(args);
		auto values = std::apply(ftn, args);
		returns(values);
	cbl.end();

	return cbl;
}

// Void functions are presumbed to contain returns(...) statements already
template <generic R, detail::acceptable_callable F>
requires (std::same_as <typename detail::signature <F> ::return_t, void>)
auto callable(F ftn)
{
	using S = detail::signature <F>;

	typename S::template manual_callable <R> cbl;
	cbl.begin();
		auto args = typename S::args_t();
		cbl.call(args);
		std::apply(ftn, args);
	cbl.end();

	return cbl;
}

// Conversion tricks inline
class callable_info {
	std::optional <std::string> name_;
public:
	callable_info() = default;
	callable_info(const std::string &name) : name_(name) {}

	auto &name(const std::string &name) {
		name_ = name;
		return *this;
	}

	friend auto operator>>(callable_info ci, auto ftn) {
		auto cbl = callable(ftn);
		if (ci.name_)
			cbl = cbl.named(*ci.name_);
		return cbl;
	}
};

template <non_trivial_generic R>
class callable_info_r {
	std::optional <std::string> name_;
public:
	callable_info_r() = default;
	callable_info_r(const std::string &name) : name_(name) {}

	auto &name(const std::string &name) {
		name_ = name;
		return *this;
	}

	friend auto operator>>(callable_info_r ci, auto ftn) {
		auto cbl = callable <R> (ftn);
		if (ci.name_)
			cbl = cbl.named(*ci.name_);
		return cbl;
	}
};

} // namespace jvl::ire
