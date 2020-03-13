#ifndef _RUA_THREAD_PA_HPP
#define _RUA_THREAD_PA_HPP

#include "basic.hpp"

#include "../sync/chan/manual.hpp"
#include "../sched/scheduler/default.hpp"

#include <functional>
#include <utility>

namespace rua {

inline void pa(std::function<void()> task) {
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
