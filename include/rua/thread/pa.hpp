#ifndef _RUA_SCHED_PA_HPP
#define _RUA_SCHED_PA_HPP

#include "basic.hpp"
#include "scheduler.hpp"

#include "../sync/chan/for_thread.hpp"

#include <functional>
#include <utility>

namespace rua {

inline void pa(std::function<void()> task) {
	static thread_chan<std::function<void()>> que;
	if (que.emplace(std::move(task))) {
		return;
	}
	thread([]() {
		for (;;) {
			que.pop()();
		}
	});
}

} // namespace rua

#endif
