#pragma once

#include "imports.hpp"
#include "intrinsics.hpp"

namespace jvl::tsg {

// Exporting the returns of a function;
// distinct types reflect parts of different
// shader stages, so parameterizing by stage
// is not required
template <typename T, typename ... Args>
void export_indexed(size_t I, T &current, Args &... args)
{
	bool layout = false;

	if constexpr (std::same_as <T, position>) {
		ire::gl_Position = current;
	} else if constexpr (ire::builtin <T>) {
		ire::layout_out <T> lout(I);
		lout = current;
		layout = true;
	}

	if constexpr (sizeof...(args))
		return export_indexed(I + layout, args...);
}

template <typename T, typename ... Args>
void exporting(T &current, Args &... args)
{
	return export_indexed(0, current, args...);
}

template <typename ... Args>
void exporting(std::tuple <Args...> &args)
{
	using ftn = void (Args &...);
	return std::apply <ftn> (exporting <Args...>, args);
}

} // namespace jvl::tsg