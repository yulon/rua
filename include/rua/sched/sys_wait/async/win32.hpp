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

class _sys_waiter {
public:
	static _sys_waiter &instance() {
		static _sys_waiter inst;
		return inst;
	}

	_sys_waiter() :
		_on_wait_ev(CreateEventW(nullptr, false, false, nullptr)),
		_tid(_make_handle_thread()) {
		assert(_on_wait_ev);
	}

	void wait(
		HANDLE handle,
		std::function<void(bool)> callback,
		ms timeout = duration_max()) {
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

		_pre_waits.emplace_front(_wait_info_t{
			h_cp,
			timeout == duration_max() ? time_max() : tick() + timeout,
			std::move(callback)});

		SetEvent(_on_wait_ev);
	}

private:
	HANDLE _on_wait_ev;
	DWORD _tid;

	struct _wait_info_t {
		HANDLE obj;
		time end_ti;
		std::function<void(bool)> cb;
	};

	lf_forward_list<_wait_info_t> _pre_waits;

	DWORD _make_handle_thread() {
		DWORD tid;
		CreateThread(nullptr, 0, &_handler, this, 0, &tid);
		assert(tid);
		return tid;
	}

	static DWORD __stdcall _handler(LPVOID lpThreadParameter) {
		auto &wtr = *reinterpret_cast<_sys_waiter *>(lpThreadParameter);

		auto is_receiving = true;
		auto need_recv = true;
		std::list<_wait_info_t> waitings;

		std::vector<HANDLE> waiting_objs;
		std::vector<typename std::list<_wait_info_t>::iterator> waiting_its;

		while (is_receiving || waitings.size()) {
			if (need_recv) {
				while (waitings.size() < MAXIMUM_WAIT_OBJECTS) {
					auto inf_opt = wtr._pre_waits.pop_front();
					if (!inf_opt) {
						break;
					}
					waitings.emplace_back(std::move(inf_opt.value()));
				}
				if (waitings.size() == MAXIMUM_WAIT_OBJECTS &&
					!wtr._pre_waits.is_empty()) {
					is_receiving = false;
					wtr._tid = wtr._make_handle_thread();
				}
				need_recv = false;
			}

			auto c = waitings.size();
			if (is_receiving) {
				++c;
			}
			waiting_objs.resize(c);
			waiting_its.resize(waiting_objs.size());

			ms timeout(duration_max());
			auto now_ti = now();

			size_t i = 0;
			if (is_receiving) {
				++i;
				waiting_objs[0] = wtr._on_wait_ev;
			}
			for (auto it = waitings.begin(); it != waitings.end();) {
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
				waiting_its[i] = it;
				++i;
				++it;
			}

			auto r = WaitForMultipleObjects(
				static_cast<DWORD>(i),
				waiting_objs.data(),
				FALSE,
				static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
					? nmax<DWORD>()
					: static_cast<DWORD>(timeout.count()));

			if (is_receiving && (r == WAIT_OBJECT_0 || r == WAIT_FAILED)) {
				need_recv = true;
				continue;
			}

			if (r == WAIT_TIMEOUT || r == WAIT_FAILED) {
				continue;
			}

			constexpr DWORD wait_object_end =
				WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS;

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
			waiting_its[r]->cb(true);
			CloseHandle(waiting_its[r]->obj);
			waitings.erase(waiting_its[r]);
		}
		return 0;
	}
};

namespace _sys_wait_async {

RUA_FORCE_INLINE void sys_wait(
	HANDLE handle,
	std::function<void(bool)> callback,
	ms timeout = duration_max()) {
	_sys_waiter::instance().wait(handle, std::move(callback), timeout);
}

} // namespace _sys_wait_async

using namespace _sys_wait_async;

}} // namespace rua::win32

#endif
