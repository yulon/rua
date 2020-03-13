#ifndef _RUA_SCHED_SCHEDULER_ABSTRACT_DEFAULT_HPP
#define _RUA_SCHED_SCHEDULER_ABSTRACT_DEFAULT_HPP

#include "abstract.hpp"

#include "../../macros.hpp"
#include "../../thread/scheduler.hpp"

#include <functional>

namespace rua {

RUA_FORCE_INLINE scheduler_i make_default_scheduler(
	std::function<scheduler_i()> make_scheduler = []() -> scheduler_i {
		return std::make_shared<thread_scheduler>();
	}) {
	static std::function<scheduler_i()> make = std::move(make_scheduler);
	return make();
}

} // namespace rua

#endif
