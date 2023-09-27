#ifndef _rua_thread_parallel_hpp
#define _rua_thread_parallel_hpp

#include "core.hpp"

#include "../invocable.hpp"
#include "../move_only.hpp"
#include "../conc/chan.hpp"
#include "../conc/future.hpp"
#include "../conc/promise.hpp"
#include "../conc/then.hpp"
#include "../conc/wait.hpp"
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
	auto prm = new newable_promise<Result>;
	auto f = make_move_only(func);
	_parallel([=]() mutable { prm->fulfill(f(args...)); });
	return *prm;
}

template <typename Func, typename... Args>
inline enable_if_t<
	std::is_void<invoke_result_t<Func, Args &&...>>::value,
	future<>>
parallel(Func func, Args &&...args) {
	auto prm = new newable_promise<>;
	auto f = make_move_only(func);
	_parallel([=]() mutable {
		f(args...);
		prm->fulfill();
	});
	return *prm;
}

} // namespace rua

#endif
