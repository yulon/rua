#ifndef _RUA_SCHED_ASYNC_HPP
#define _RUA_SCHED_ASYNC_HPP

#include "../thread/basic.hpp"

#include "../sync/chan.hpp"
#include "../types/util.hpp"

#include <functional>

namespace rua {

template <typename Callee, typename Ret = decltype(std::declval<Callee &&>()())>
inline enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		std::is_same<Ret, void>::value,
	Ret>
async(Callee &&task) {
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

template <typename Ret, typename... Args>
inline Ret async(Ret (&callee)(Args...)) {
	return async(&callee);
}

} // namespace rua

#endif
