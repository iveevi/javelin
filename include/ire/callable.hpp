#pragma once

#include "../atom/atom.hpp"
#include "../atom/kernel.hpp"
#include "primitive.hpp"
#include "uniform_layout.hpp"
#include "type_synthesis.hpp"
#include "emitter.hpp"
#include <type_traits>

namespace jvl::ire {

struct Callable {
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

	// Ordinary information, same as Emitter
	// but lacks the used and synthesized members
	std::vector <atom::General> pool;
	size_t pointer;
	size_t cid;

	// An optional name
	std::string name;

	// For callables we can track back used and synthesized
	// insructions from working backwards at the returns

	Callable();
	Callable(const Callable &);
	Callable &operator=(const Callable &);

	// TODO: destructor, which offloads it from the global list

	atom::Kernel export_to_kernel();

	int emit(const atom::General &);

	void dump();
};

// Internal construction of callables
// TODO: type restrictions with concepts
template <typename R, typename ... Args>
struct callable_t : Callable {
	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl) {
		auto &em = Emitter::active;

		using type_t = std::decay_t <decltype(std::get <index> (tpl))>;

		if constexpr (uniform_compatible <type_t>) {
			auto &x = std::get <index> (tpl);

			auto layout = x.layout().remove_const();

			atom::Global global;
			global.qualifier = atom::Global::parameter;
			global.type = type_field_from_args(layout).id;
			global.binding = index;

			cache_index_t ref;
			ref = em.emit(global);
			layout.__ref_with(ref);
		} else {
			auto &x = std::get <index> (tpl);

			using type_of_x = std::decay_t <decltype(x)>;

			atom::Global global;
			global.qualifier = atom::Global::parameter;
			global.type = type_field_from_args <type_of_x> ().id;
			global.binding = index;

			x.ref = em.emit(global);
		}

		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	void call(std::tuple <Args...> &args) {
		Emitter::active.scopes.push(*this);
		__fill_parameter_references <0> (args);
	}

	auto &end() {
		Emitter::active.scopes.pop();
		return *this;
	}

	auto &named(const std::string &name_) {
		name = name_;
		return *this;
	}

	R operator()(const Args &... args) {
		auto &em = Emitter::active;

		atom::Call call;
		call.cid = cid;
		call.ret = type_field_from_args <R> ().id;
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

template <typename R, typename ... Args>
struct signature_pair {
	using return_t = R;
	using args_t = std::tuple <std::decay_t <Args>...>;

	using callable = callable_t <R, std::decay_t <Args>...>;

	template <typename RR>
	using manual_callable = callable_t <RR, std::decay_t <Args>...>;
};

template <typename R, typename ... Args>
auto function_signature(const std::function <R (Args...)> &) -> signature_pair <R, Args...>
{
	return {};
}

template <acceptable_callable F>
auto function_signature(const F &ftn)
{
	return function_signature(std::function(ftn));
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
template <typename R, typename ... Args>
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
	auto args = typename S::args_t();
	cbl.call(args);
	auto values = std::apply(ftn, args);
	returns(values);
	return cbl.end();
}

// Void functions are presumbed to contain returns(...) statements already
template <typename R, detail::acceptable_callable F>
requires (std::same_as <typename detail::signature <F> ::return_t, void>)
auto callable(F ftn)
{
	using S = detail::signature <F>;

	typename S::template manual_callable <R> cbl;
	auto args = typename S::args_t();
	cbl.call(args);
	std::apply(ftn, args);
	return cbl.end();
}

} // namespace jvl::ire
