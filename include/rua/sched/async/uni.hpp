#ifndef _RUA_SCHED_ASYNC_UNI_HPP
#define _RUA_SCHED_ASYNC_UNI_HPP

#include "../../thread/basic.hpp"

#include "../../sync/chan.hpp"
#include "../../types/util.hpp"

#include <functional>

namespace rua {

inline void async(std::function<void()> task) {
	static chan<std::function<void()>> que;
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
