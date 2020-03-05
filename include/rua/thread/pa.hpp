#ifndef _RUA_SCHED_PA_HPP
#define _RUA_SCHED_PA_HPP

#include "basic.hpp"
#include "scheduler.hpp"

#include "../sync/chan/basic.hpp"

#include <functional>
#include <utility>

namespace rua {

inline void pa(std::function<void()> task) {
	static chan<std::function<void()>> que;
	if (que.emplace(std::move(task))) {
		return;
	}
	thread([]() {
		auto &&sch = thread_scheduler();
		for (;;) {
			que.pop(sch)();
		}
	});
}

} // namespace rua

#endif
