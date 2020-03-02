#ifndef _RUA_SCHED_PA_HPP
#define _RUA_SCHED_PA_HPP

#include "basic.hpp"
#include "scheduler.hpp"

#include "../macros.hpp"
#include "../sync/chan/basic.hpp"

#include <functional>
#include <utility>

namespace rua {

RUA_FORCE_INLINE chan<std::function<void()>> &_pa_tsk_que() {
	static chan<std::function<void()>> que;
	return que;
}

RUA_FORCE_INLINE void _pa_make_thread() {
	thread([]() {
		auto &&sch = thread_scheduler();
		auto &que = _pa_tsk_que();
		for (;;) {
			que.pop(sch)();
		}
	});
}

RUA_FORCE_INLINE void pa(std::function<void()> tsk) {
	if (_pa_tsk_que().emplace(std::move(tsk))) {
		return;
	}
	_pa_make_thread();
}

} // namespace rua

#endif
