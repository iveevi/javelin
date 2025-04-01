#pragma once

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

template <ExecutionFlag Flag, resource ... Resources>
bool rexec_pass(const ResourceExecutionContext <Flag, Resources...> &);

template <typename T>
concept rexec_class = requires(const T &x) {
	{ rexec_pass(x) } -> std::same_as <bool>;
};

///////////////////
// REXEC methods //
///////////////////

#define $use(id) _use <id> ()

// $rec_... variants are simply static inline qualified
#define $rexec_subroutine(...)	static inline $subroutine(__VA_ARGS__)
#define $rexec_entrypoint(...)	static inline $entrypoint(__VA_ARGS__)

#define $declare_subroutine(R, name, ...)					\
	::jvl::ire::manifest_skeleton <R, void (*)(__VA_ARGS__)> ::proc name

#define $declare_rexec_subroutine(...)	static $declare_subroutine(__VA_ARGS__)

#define $implement_rexec_subroutine(rexec, R, name, ...)				\
	::jvl::ire::manifest_skeleton <R, void (*)(__VA_ARGS__)> ::proc rexec::name	\
		= ::jvl::ire::ProcedureBuilder <R> (#name)				\
		<< [_returner = _return_igniter <R> ()](__VA_ARGS__) -> void

} // namespace jvl::rexec