#include "common/debug.hpp"

namespace jvl::ire {

namespace detail {

std::filesystem::path trace_destination = ".javelin";

} // namespace detail

void set_trace_destination(const std::filesystem::path &path)
{
	detail::trace_destination = path;
}

} // namespace jvl::ire