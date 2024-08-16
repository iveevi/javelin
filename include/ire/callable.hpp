#pragma once

#include "../atom/atom.hpp"
#include "../atom/kernel.hpp"
#include "ire/uniform_layout.hpp"
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
// TODO: restrict argument types...
template <typename R, typename ... Args>
struct __callable : Callable {
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
