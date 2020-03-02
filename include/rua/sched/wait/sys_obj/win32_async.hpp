#ifndef _RUA_SCHED_WAIT_SYS_OBJ_ASYNC_HPP
#define _RUA_SCHED_WAIT_SYS_OBJ_ASYNC_HPP

#include "../../../chrono.hpp"
#include "../../../macros.hpp"
#include "../../../sync/lf_forward_list.hpp"
#include "../../../thread/pa.hpp"

#include <windows.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <utility>
#include <vector>

namespace rua { namespace win32 {

struct _wait_info_t {
	HANDLE obj;
	time end_ti;
	std::function<void(bool)> cb;
};

struct _sys_obj_waiter_ctx_t {
	lf_forward_list<_wait_info_t> pre_waits;
	std::atomic<size_t> wait_c;
	std::atomic<size_t> waiter_c;
	HANDLE ev;
};

RUA_FORCE_INLINE _sys_obj_waiter_ctx_t &_sys_obj_waiter_ctx() {
	static _sys_obj_waiter_ctx_t ctx;
	return ctx;
}

inline void _sys_obj_waiter() {
	auto &ctx = _sys_obj_waiter_ctx();

	std::list<_wait_info_t> waitings;

	HANDLE waiting_objs[MAXIMUM_WAIT_OBJECTS];
	RUA_SPASSERT(sizeof(waiting_objs) < 1024);

	int waited_ix = -1;

	for (;;) {
		while (waited_ix > -1 ? waitings.size() - 1
							  : waitings.size() < MAXIMUM_WAIT_OBJECTS) {
			auto inf_opt = ctx.pre_waits.pop_front();
			if (!inf_opt) {
				break;
			}
			waitings.emplace_back(std::move(inf_opt.value()));
		}

		ms timeout(duration_max());
		auto now_ti = now();

		int before_i = 0;
		size_t i = 0;
		for (auto it = waitings.begin(); it != waitings.end(); ++before_i) {
			if (before_i == waited_ix) {
				waited_ix = -1;
				it->cb(true);
				CloseHandle(it->obj);
				it = waitings.erase(it);
				continue;
			}
			if (now_ti >= it->end_ti) {
				it->cb(false);
				CloseHandle(it->obj);
				it = waitings.erase(it);
				continue;
			}
			auto this_timeout = it->end_ti - now_ti;
			if (this_timeout < timeout) {
				timeout = this_timeout;
			}
			waiting_objs[i] = it->obj;
			++i;
			++it;
		}
		auto c = waitings.size();
		auto is_full = c == MAXIMUM_WAIT_OBJECTS;
		if (!is_full) {
			assert(i == c);
			waiting_objs[c] = ctx.ev;
			++i;
		}

		auto r = WaitForMultipleObjects(
			static_cast<DWORD>(i),
			&waiting_objs[0],
			FALSE,
			static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
				? nmax<DWORD>()
				: static_cast<DWORD>(timeout.count()));

		if ((!is_full && r == WAIT_OBJECT_0 + c) || r == WAIT_TIMEOUT ||
			r == WAIT_FAILED) {
			continue;
		}

		constexpr DWORD wait_object_end = WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS;

		constexpr DWORD wait_abandoned_end =
			WAIT_ABANDONED_0 + MAXIMUM_WAIT_OBJECTS;

		if (r >= wait_object_end && r < WAIT_ABANDONED_0) {
			continue;
		}
		if (r >= WAIT_ABANDONED_0) {
			if (r >= wait_abandoned_end) {
				continue;
			}
			r -= WAIT_ABANDONED_0;
		}
		waited_ix = r;
	}
}

namespace _wait_sys_obj_async {

RUA_FORCE_INLINE void wait(
	HANDLE handle,
	std::function<void(bool)> callback,
	ms timeout = duration_max()) {

	auto &ctx = _sys_obj_waiter_ctx();

	assert(handle);

	HANDLE h_cp;
	DuplicateHandle(
		GetCurrentProcess(),
		handle,
		GetCurrentProcess(),
		&h_cp,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);
	assert(h_cp);

	++ctx.wait_c;
	ctx.pre_waits.emplace_front(
		_wait_info_t{h_cp,
					 timeout == duration_max() ? time_max() : tick() + timeout,
					 std::move(callback)});

	for (;;) {
		auto wait_c = ctx.wait_c.load();
		auto waiter_max = wait_c / 64;
		if (wait_c % 64) {
			++waiter_max;
		}
		auto waiter_c = ctx.waiter_c.load();
		if (waiter_c >= waiter_max) {
			break;
		}
		if (ctx.waiter_c.compare_exchange_weak(waiter_c, waiter_c + 1)) {
			pa(_sys_obj_waiter);
			return;
		}
	}

	SetEvent(ctx.ev);
}

} // namespace _wait_sys_obj_async

using namespace _wait_sys_obj_async;

}} // namespace rua::win32

#endif
