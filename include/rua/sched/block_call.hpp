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

RUA_FORCE_INLINE void _make_block_caller() {
	thread([]() {
		auto &que = _block_call_que();
		for (;;) {
			que.pop()();
		}
	});
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &>()(std::declval<Args &>()...))>
enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		std::is_same<Ret, void>::value,
	Ret>
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
		_make_block_caller();
	}
	sch->wait();
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &>()(std::declval<Args &>()...))>
enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		!std::is_same<Ret, void>::value,
	Ret>
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
		_make_block_caller();
	}
	sch->wait();
	auto r = *r_ptr;
	delete r_ptr;
	return r;
}

template <typename Ret, typename... Params, typename... Args>
Ret block_call(Ret (&callee)(Params...), Args &&... args) {
	return block_call(&callee, std::forward<Args>(args)...);
}

} // namespace rua

#endif
