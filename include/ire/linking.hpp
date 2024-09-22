#pragma once

#include "../thunder/linkage_unit.hpp"

namespace jvl::ire {

// Convenience methods to generate linked unit
inline thunder::LinkageUnit link(const std::vector <thunder::TrackedBuffer> &tracked)
{
	thunder::LinkageUnit unit;
	for (auto &tb : tracked)
		unit.add(tb);

	return unit;
}

template <typename ... Args>
inline thunder::LinkageUnit link(const Args &... args)
{
	std::vector <thunder::TrackedBuffer> tracked { args... };

	thunder::LinkageUnit unit;
	for (auto &tb : tracked)
		unit.add(tb);

	return unit;
}

} // namespace jvl::ire