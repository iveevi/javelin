#pragma once

#include "type_system.hpp"
#include "../callable.hpp"
#include "../../thunder/ad.hpp"

namespace jvl::ire {

template <typename R, typename ... Args>
requires differentiable_type <R> && (differentiable_type <Args> && ...)
auto dfwd(const callable_t <R, Args...> &callable)
{
	callable_t <dual_t <R>, dual_t <Args>...> result;
	thunder::ad_fwd_transform(result, callable);
	return result.named("__dfwd_" + callable.name);
}

} // namespace jvl::ire