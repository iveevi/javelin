#pragma once

#include "../atom/atom.hpp"
#include "../atom/kernel.hpp"
#include "type_synthesis.hpp"
#include "emitter.hpp"

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
template <typename R, typename ... Args>
struct __callable : Callable {
	std::tuple <Args...> mimic;
	std::tuple <Args *...> passed;

	template <size_t index>
	void __fill_parameter_references(std::tuple <Args...> &tpl) {
		std::get <index> (passed) = &std::get <index> (tpl);
		if constexpr (index + 1 < sizeof...(Args))
			__fill_parameter_references <index + 1> (tpl);
	}

	template <size_t index>
	void __transfer_mimic_references() {
		auto src = std::get <index> (mimic);
		auto dst = std::get <index> (passed);
		dst->ref = src.ref;

		if constexpr (index + 1 < sizeof...(Args))
			__transfer_mimic_references <index + 1> ();
	}

	// TODO: only do this if the argument is a synthesizable
	// TODO: how to deal with passing global variables
	// (push constants/layouts with custom layouts?)
	// TODO: constexpr switches to handle each case...
	template <size_t index>
	void __fill_mimic_references() {
		auto &x = std::get <index> (mimic);

		auto &em = Emitter::active;

		using type_of_x = std::decay_t <decltype(x)>;

		atom::Global global;
		global.qualifier = atom::Global::parameter;
		global.type = type_field_from_args <type_of_x> ().id;
		global.binding = index;

		x.ref = em.emit(global);

		if constexpr (index + 1 < sizeof...(Args))
			__fill_mimic_references <index + 1> ();
	}

	void call(std::tuple <Args...> &args) {
		Emitter::active.scopes.push(*this);
		__fill_parameter_references <0> (args);
		__fill_mimic_references <0> ();
		__transfer_mimic_references <0> ();
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

// Metaprogramming hacks to get function arguments more easily
template <typename R, typename ... Args>
struct __callable_redirect {
	using type = __callable <R, Args...>;
};

template <typename R, typename ... Args>
struct __callable_redirect <R, std::tuple <Args...>> {
	using type = __callable <R, Args...>;
};

template <typename F>
struct signature;

template <typename R, typename ... Args>
struct signature <R (Args...)> {
	using args_t = std::tuple <Args...>;
	using return_t = R;
};

template <typename R, typename F>
using __callable_t = __callable_redirect <R, typename signature <F> ::args_t> ::type;

// User end function to create a callable from a regular C++ function
template <typename R, typename F>
requires std::is_function_v <F>
__callable_t <R, F>
callable(const F &ftn)
{
	__callable_t <R, F> cbl;
	auto args = typename signature <F> ::args_t();
	cbl.call(args);
	std::apply(ftn, args);
	return cbl.end();
}

} // namespace jvl::ire
