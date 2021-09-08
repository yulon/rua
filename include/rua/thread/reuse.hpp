#ifndef _RUA_THREAD_REUSE_HPP
#define _RUA_THREAD_REUSE_HPP

#include "basic.hpp"

#include "../sync/chan.hpp"
#include "../types/util.hpp"

#include <functional>

namespace rua {

inline void reuse_thread(std::function<void()> f) {
	static chan<std::function<void()>> que;
	if (que.emplace(std::move(f))) {
		return;
	}
	thread([]() {
		for (;;) {
			que.pop()();
		}
	});
}

template <typename F, typename... Args>
inline enable_if_t<(sizeof...(Args) > 0) && is_invocable<F, Args...>::value>
reuse_thread(F &&f, Args &&...args) {
	reuse_thread([=]() mutable { f(args...); });
}

} // namespace rua

#endif
