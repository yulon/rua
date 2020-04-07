#ifndef _RUA_SCHED_REG_WAIT_WIN32_HPP
#define _RUA_SCHED_REG_WAIT_WIN32_HPP

#include "../../chrono.hpp"
#include "../../macros.hpp"
#include "../../types/util.hpp"

#include <cassert>
#include <functional>

#ifndef RUA_USE_BUILTIN_REG_WAIT

#include <winbase.h>

namespace rua { namespace win32 {

struct _reg_wait_sys_h_ctx_t {
	HANDLE wait_h;
	std::function<void(bool)> cb;
};

inline VOID CALLBACK _reg_wait_sys_h_cb(PVOID _ctx, BOOLEAN timeouted) {
	auto ctx = reinterpret_cast<_reg_wait_sys_h_ctx_t *>(_ctx);
	ctx->cb(!timeouted);
	UnregisterWaitEx(ctx->wait_h, nullptr);
	delete ctx;
}

namespace _reg_wait {

RUA_FORCE_INLINE void reg_wait(
	HANDLE handle,
	std::function<void(bool)> callback,
	ms timeout = duration_max()) {

	assert(handle);

	auto ctx = new _reg_wait_sys_h_ctx_t{nullptr, std::move(callback)};

	RegisterWaitForSingleObject(
		&ctx->wait_h,
		handle,
		_reg_wait_sys_h_cb,
		ctx,
		timeout == duration_max()
			? INFINITE
			: static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
				  ? INFINITE
				  : static_cast<DWORD>(timeout.count()),
		WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
}

} // namespace _reg_wait

}} // namespace rua::win32

#else

#include "../../sync/lockfree_list.hpp"
#include "../../thread/basic/win32.hpp"

#include <windows.h>

#include <atomic>
#include <list>

namespace rua { namespace win32 {

class _sys_h_waiter {
public:
	static _sys_h_waiter &instance() {
		static _sys_h_waiter inst;
		return inst;
	}

	_sys_h_waiter() :
		_wait_c(0),
		_td_c(0),
		_on_wait(CreateEventW(nullptr, false, false, nullptr)) {}

	size_t calc_waiter_max() const {
		auto wait_c = _wait_c.load();
		auto waiter_max = wait_c / 64;
		if (wait_c % 64) {
			++waiter_max;
		}
		return waiter_max;
	}

	void reg_wait(
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
			auto td_c = _td_c.load();
			if (td_c >= calc_waiter_max()) {
				break;
			}
			auto td_c_inc = td_c + 1;
			if (_td_c.compare_exchange_weak(td_c, td_c_inc)) {
				thread([this]() { waits(); });
				return;
			}
		}

		SetEvent(_on_wait);
	}

	void waits() {
		std::list<_wait_t> waitings;

		HANDLE waiting_hs[MAXIMUM_WAIT_OBJECTS];
		RUA_SPASSERT(sizeof(waiting_hs) < 1024);

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
			auto now_ti = tick();

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
				waiting_hs[i] = it->h;
				++i;
				++it;
			}
			auto c = waitings.size();
			auto is_max = c == MAXIMUM_WAIT_OBJECTS;
			if (!is_max) {
				assert(i == c);
				waiting_hs[c] = _on_wait;
				++i;
			}

			auto r = WaitForMultipleObjects(
				static_cast<DWORD>(i),
				&waiting_hs[0],
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
	std::atomic<size_t> _wait_c, _td_c;
	HANDLE _on_wait;
};

namespace _reg_wait {

RUA_FORCE_INLINE void reg_wait(
	HANDLE handle,
	std::function<void(bool)> callback,
	ms timeout = duration_max()) {
	_sys_h_waiter::instance().reg_wait(handle, std::move(callback), timeout);
}

} // namespace _reg_wait

}} // namespace rua::win32

#endif

namespace rua { namespace win32 {

using namespace _reg_wait;

}} // namespace rua::win32

#endif
