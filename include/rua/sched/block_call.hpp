#ifndef _RUA_SCHED_BLOCK_CALL_HPP
#define _RUA_SCHED_BLOCK_CALL_HPP

#include "../sched/util.hpp"
#include "../sync/chan.hpp"
#include "../thread/creational.hpp"
#include "../thread/scheduler.hpp"
#include "../type_traits.hpp"

namespace rua {

inline chan<std::function<void()>> &_block_call_que() {
	static chan<std::function<void()>> que;
	return que;
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decay_t<decltype(std::declval<Callee &>()(std::declval<Args &>()...))>>
enable_if_t<std::is_same<Ret, void>::value, Ret>
block_call(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		callee(std::forward<Args>(args)...);
		return;
	}
	auto sig = sch->get_signaler();
	if (!_block_call_que().emplace([=]() mutable {
			callee(args...);
			sig->signal();
		})) {
		thread([]() {
			auto &que = _block_call_que();
			for (;;) {
				que.pop()();
			}
		});
	}
	sch->wait();
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decay_t<decltype(std::declval<Callee &>()(std::declval<Args &>()...))>>
enable_if_t<!std::is_same<Ret, void>::value, Ret>
block_call(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		return callee(std::forward<Args>(args)...);
	}
	auto sig = sch->get_signaler();
	auto r_ptr = new Ret;
	if (_block_call_que().emplace([=]() mutable {
			*r_ptr = callee(args...);
			sig->signal();
		})) {
		thread([]() {
			auto &que = _block_call_que();
			for (;;) {
				que.pop()();
			}
		});
	}
	sch->wait();
	auto r = *r_ptr;
	delete r_ptr;
	return r;
}

} // namespace rua

#endif
