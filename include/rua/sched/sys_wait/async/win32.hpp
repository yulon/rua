#ifndef _RUA_SCHED_SYS_WAIT_ASYNC_WIN32_HPP
#define _RUA_SCHED_SYS_WAIT_ASYNC_WIN32_HPP

#include "../../../chrono.hpp"
#include "../../../sync/lf_forward_list.hpp"

#include <windows.h>

#include <cassert>
#include <functional>
#include <list>
#include <utility>
#include <vector>

namespace rua { namespace win32 {

struct _sys_wait_info_t {
	HANDLE obj;
	time end_ti;
	std::function<void(bool)> cb;
};

struct _sys_waiter_ctx_t {
	DWORD tid;
	HANDLE sig;
	lf_forward_list<_sys_wait_info_t> waits;
	std::list<_sys_wait_info_t> waitings;
};

inline DWORD __stdcall _sys_waiter(LPVOID lpThreadParameter) {
	auto &ctx = *reinterpret_cast<_sys_waiter_ctx_t *>(lpThreadParameter);

	std::vector<HANDLE> objs{ctx.sig};
	std::vector<typename std::list<_sys_wait_info_t>::iterator> its{
		ctx.waitings.end()};

	for (;;) {
		auto c = ctx.waitings.size() + 1;
		objs.resize(c > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : c);
		its.resize(objs.size());

		auto timeout = INFINITE;
		auto now_ti = now();

		size_t i = 1;
		for (auto it = ctx.waitings.begin(); it != ctx.waitings.end();) {
			if (now_ti >= it->end_ti) {
				it->cb(false);
				it = ctx.waitings.erase(it);
				continue;
			}
			auto this_timeout = ms(it->end_ti - now_ti).count();
			if (this_timeout < timeout) {
				timeout = static_cast<DWORD>(this_timeout);
			}
			if (i > c) {
				++it;
				continue;
			}
			objs[i] = it->obj;
			its[i] = it;
			++i;
			++it;
		}

		auto r = WaitForMultipleObjects(
			static_cast<DWORD>(objs.size()), objs.data(), FALSE, timeout);

		assert(r != WAIT_FAILED);

		if (r == WAIT_OBJECT_0) {
			for (;;) {
				auto inf_opt = ctx.waits.pop_front();
				if (!inf_opt) {
					break;
				}
				ctx.waitings.emplace_back(std::move(inf_opt.value()));
			}
			continue;
		}

		if (r == WAIT_TIMEOUT) {
			continue;
		}

		if (r >= WAIT_ABANDONED_0) {
			r -= WAIT_ABANDONED_0;
		}
		its[r]->cb(true);
		ctx.waitings.erase(its[r]);
	}
	return 0;
}

namespace _sys_wait_async {

inline void
sys_wait(HANDLE handle, ms timeout, std::function<void(bool)> callback) {
	assert(handle);

	static _sys_waiter_ctx_t &ch_ref = ([]() -> _sys_waiter_ctx_t & {
		static _sys_waiter_ctx_t ch;
		ch.sig = CreateEventW(nullptr, false, false, nullptr);
		assert(ch.sig);
		CreateThread(nullptr, 0, &_sys_waiter, &ch, 0, &ch.tid);
		assert(ch.tid);
		return ch;
	})();

	auto inf = _sys_wait_info_t{handle,
								timeout == duration_max() ? time_max()
														  : tick() + timeout,
								std::move(callback)};
	if (GetCurrentThreadId() == ch_ref.tid) {
		ch_ref.waitings.emplace_back(std::move(inf));
	}
	ch_ref.waits.emplace_front(std::move(inf));
	SetEvent(ch_ref.sig);
}

RUA_FORCE_INLINE void sys_wait(HANDLE handle, std::function<void()> callback) {
	sys_wait(handle, duration_max(), [callback](bool) { callback(); });
}

} // namespace _sys_wait_async

using namespace _sys_wait_async;

}} // namespace rua::win32

#endif
