#ifndef _RUA_SCHED_WAIT_SYS_OBJ_WIN32_HPP
#define _RUA_SCHED_WAIT_SYS_OBJ_WIN32_HPP

#include "../../scheduler/this_decl.hpp"

#include "../../../chrono.hpp"
#include "../../../macros.hpp"
#include "../../../sync/chan.hpp"
#include "../../../sync/lockfree_list.hpp"
#include "../../../thread/basic/win32.hpp"
#include "../../../thread/scheduler.hpp"
#include "../../../types/util.hpp"

#include <windows.h>

#include <atomic>
#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <vector>

namespace rua { namespace win32 {

class _sys_obj_waiter_pool {
public:
	static _sys_obj_waiter_pool &instance() {
		static _sys_obj_waiter_pool inst;
		return inst;
	}

	_sys_obj_waiter_pool() :
		_wait_c(0),
		_waiter_c(0),
		_on_wait(CreateEventW(nullptr, false, false, nullptr)) {}

	size_t calc_waiter_max() const {
		auto wait_c = _wait_c.load();
		auto waiter_max = wait_c / 64;
		if (wait_c % 64) {
			++waiter_max;
		}
		return waiter_max;
	}

	void wait(
		HANDLE handle,
		std::function<void(bool)> callback,
		ms timeout = duration_max()) {

		assert(handle);

		++_wait_c;

		_pre_waits.emplace_front(
			_wait_t{handle,
					timeout == duration_max() ? time_max() : tick() + timeout,
					std::move(callback)});

		for (;;) {
			auto waiter_c = _waiter_c.load();
			if (waiter_c >= calc_waiter_max()) {
				break;
			}
			auto waiter_c_inc = waiter_c + 1;
			if (_waiter_c.compare_exchange_weak(waiter_c, waiter_c_inc)) {
				thread([this, waiter_c_inc]() { waits(waiter_c_inc); });
				return;
			}
		}

		SetEvent(_on_wait);
	}

	void waits(size_t waiter_id) {
		std::list<_wait_t> waitings;

		HANDLE waiting_objs[MAXIMUM_WAIT_OBJECTS];
		RUA_SPASSERT(sizeof(waiting_objs) < 1024);

		int waited_ix = -1;

		for (;;) {
			if (waitings.size() < MAXIMUM_WAIT_OBJECTS) {
				auto pre_waits = _pre_waits.pop_all();
				if (pre_waits) {
					while (pre_waits &&
						   waitings.size() < MAXIMUM_WAIT_OBJECTS) {
						waitings.emplace_back(pre_waits.pop_front());
					}
					if (pre_waits) {
						_pre_waits.prepend(std::move(pre_waits));
						SetEvent(_on_wait);
					}
				}
			}

			ms timeout(duration_max());
			auto now_ti = now();

			int before_i = 0;
			size_t i = 0;
			for (auto it = waitings.begin(); it != waitings.end(); ++before_i) {
				if (before_i == waited_ix) {
					--_wait_c;
					waited_ix = -1;
					it->cb(true);
					CloseHandle(it->h);
					it = waitings.erase(it);
					continue;
				}
				if (now_ti >= it->end_ti) {
					--_wait_c;
					it->cb(false);
					CloseHandle(it->h);
					it = waitings.erase(it);
					continue;
				}
				auto this_timeout = it->end_ti - now_ti;
				if (this_timeout < timeout) {
					timeout = this_timeout;
				}
				waiting_objs[i] = it->h;
				++i;
				++it;
			}
			auto c = waitings.size();
			auto is_max = c == MAXIMUM_WAIT_OBJECTS;
			if (!is_max) {
				assert(i == c);
				waiting_objs[c] = _on_wait;
				++i;
			}

			auto r = WaitForMultipleObjects(
				static_cast<DWORD>(i),
				&waiting_objs[0],
				FALSE,
				static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
					? nmax<DWORD>()
					: static_cast<DWORD>(timeout.count()));

			if ((!is_max && r == WAIT_OBJECT_0 + c) || r == WAIT_TIMEOUT ||
				r == WAIT_FAILED) {
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
			waited_ix = r;
		}
	}

private:
	struct _wait_t {
		HANDLE h;
		time end_ti;
		std::function<void(bool)> cb;
	};

	lockfree_list<_wait_t> _pre_waits;
	std::atomic<size_t> _wait_c, _waiter_c;
	HANDLE _on_wait;
};

namespace _wait_sys_obj {

RUA_FORCE_INLINE void wait(
	HANDLE handle,
	std::function<void(bool)> callback,
	ms timeout = duration_max()) {
	_sys_obj_waiter_pool::instance().wait(handle, std::move(callback), timeout);
}

inline bool wait(HANDLE handle, ms timeout = duration_max()) {
	assert(handle);

	auto sch = this_scheduler();
	if (sch.type_is<rua::thread_scheduler>()) {
		return WaitForSingleObject(
				   handle,
				   static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
					   ? nmax<DWORD>()
					   : static_cast<DWORD>(timeout.count())) != WAIT_TIMEOUT;
	}
	auto ch = std::make_shared<chan<bool>>();
	wait(
		handle, [=](bool r) mutable { *ch << r; }, timeout);
	return ch->pop(sch);
}

} // namespace _wait_sys_obj

using namespace _wait_sys_obj;

}} // namespace rua::win32

#endif
