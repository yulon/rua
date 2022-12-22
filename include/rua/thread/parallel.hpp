#ifndef _RUA_THREAD_PARALLEL_HPP
#define _RUA_THREAD_PARALLEL_HPP

#include "core.hpp"

#include "../skater.hpp"
#include "../sync/chan.hpp"
#include "../sync/promise.hpp"
#include "../sync/then.hpp"
#include "../sync/wait.hpp"
#include "../util.hpp"

#include <functional>
#include <iostream>

namespace rua {

inline void _parallel(std::function<void()> f) {
	static chan<std::function<void()>> que;
	if (que.send(std::move(f))) {
		return;
	}
	thread([]() {
		for (;;) {
			auto f = **que.recv();
			f();
		}
	});
}

template <
	typename Func,
	typename... Args,
	typename Result = invoke_result_t<Func, Args &&...>>
inline enable_if_t<!std::is_void<Result>::value, future<Result>>
parallel(Func func, Args &&...args) {
	skater<promise<Result>> prm;
	auto prm_fut = prm->get_promising_future();
	auto f = skate(func);
	_parallel([=]() mutable { prm->deliver(f(args...)); });
	return std::move(prm_fut);
}

template <typename Func, typename... Args>
inline enable_if_t<
	std::is_void<invoke_result_t<Func, Args &&...>>::value,
	future<>>
parallel(Func func, Args &&...args) {
	skater<promise<>> prm;
	auto prm_fut = prm->get_promising_future();
	auto f = skate(func);
	_parallel([=]() mutable {
		f(args...);
		prm->deliver();
	});
	return std::move(prm_fut);
}

} // namespace rua

#endif
