#ifndef _RUA_THREAD_PA_HPP
#define _RUA_THREAD_PA_HPP

#include "basic.hpp"

#include "../sched/scheduler/default.hpp"
#include "../sync/chan.hpp"
#include "../types/util.hpp"

#include <functional>

namespace rua {

RUA_FORCE_INLINE void pa(std::function<void()> task) {
	static chan<std::function<void()>> que;
	if (que.emplace(std::move(task))) {
		return;
	}
	thread([]() {
		auto sch = make_default_scheduler();
		for (;;) {
			que.pop(sch)();
		}
	});
}

} // namespace rua

#endif
