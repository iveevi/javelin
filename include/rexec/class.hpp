#pragma once

#include "../ire/procedure/ordinary.hpp"
#include "collection.hpp"
#include "manifestation.hpp"

namespace jvl::rexec {

////////////////////////////////////
// Resource and execution context //
////////////////////////////////////

enum class ExecutionFlag : uint8_t {
	eInvalid,
	eSubroutine,
	eVertex,
	eFragment,
};

template <ExecutionFlag Flag, resource ... Resources>
struct ResourceExecutionContext : resource_manifestion <Resources...> {
	static constexpr ExecutionFlag flag = Flag;

	using collection = resource_collection <Resources...>;
};

// Subtypes
template <typename R, typename ... Args>
struct rexec_method : ire::Procedure <Args...> {
	using rexec = R;

	rexec_method(const ire::Procedure <Args...> &proc)
			: ire::Procedure <Args...> (proc) {}
};

//////////////
// Concepts //
//////////////

template <ExecutionFlag Flag, resource ... Resources>
bool rexec_pass(const ResourceExecutionContext <Flag, Resources...> &);

template <typename T>
concept rexec_class = requires(const T &x) {
	{ rexec_pass(x) } -> std::same_as <bool>;
};

template <typename R, typename ... Args>
bool rexec_method_pass(const rexec_method <R, Args...> &);

template <typename T>
concept rexec_class_method = requires(const T &value) {
	{ rexec_method_pass(value) } -> std::same_as <bool>;
};

///////////////////
// REXEC methods //
///////////////////

#define $use(id) _use <id> ()

// $rec_... variants are inlined and unique to each rexec
// also includes metadata (type) for overhead context
#define $rexec_entrypoint(name)						\
	static inline rexec_method <self, void> name			\
		= ::jvl::ire::ProcedureBuilder("main")			\
		<< typename self::_initializer()			\
		* [_returner = _return_igniter <void> ()]() -> void

template <typename R, ire::generic_or_void Ret, typename F>
struct rexec_manifest_skeleton {
	using proc = void;
};

template <typename R, ire::generic_or_void Ret, typename ... Args>
struct rexec_manifest_skeleton <R, Ret, void (*)(Args...)> {
	using proc = rexec_method <R, Ret, Args...>;
};

#define $rexec_subroutine(R, name, ...)								\
	static inline rexec_manifest_skeleton <self, R, void (*)(__VA_ARGS__)> ::proc name	\
		= ::jvl::ire::ProcedureBuilder <R> (#name)					\
		<< typename self::_initializer()						\
		* [_returner = _return_igniter <R> ()](__VA_ARGS__) -> void


// TODO: partial rexec methods...

} // namespace jvl::rexec