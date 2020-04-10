#ifndef _RUA_THREAD_SCHEDULER_WIN32_HPP
#define _RUA_THREAD_SCHEDULER_WIN32_HPP

#include "../../macros.hpp"
#include "../../sched/scheduler/abstract.hpp"
#include "../../types/util.hpp"

#include <windows.h>

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

class thread_waker : public waker {
public:
	using native_handle_t = HANDLE;

	thread_waker() : _h(CreateEventW(nullptr, false, false, nullptr)) {}

	virtual ~thread_waker() {
		if (!_h) {
			return;
		}
		CloseHandle(_h);
		_h = nullptr;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	operator bool() const {
		return _h;
	}

	virtual void wake() {
		SetEvent(_h);
	}

	void reset() {
		ResetEvent(_h);
	}

private:
	HANDLE _h;
};

class thread_scheduler : public scheduler {
public:
	constexpr thread_scheduler(duration yield_dur = 0) :
		_yield_dur(yield_dur), _wkr() {}

	virtual ~thread_scheduler() = default;

	virtual void yield() {
		if (_yield_dur > 1) {
			sleep(_yield_dur);
			return;
		}
		for (auto i = 0; i < 3; ++i) {
			if (SwitchToThread()) {
				return;
			}
		}
		Sleep(1);
	}

	virtual bool sleep(duration timeout, bool wakeable = false) {
		if (!wakeable) {
			Sleep(timeout.milliseconds<DWORD, INFINITE>());
			return false;
		}
		assert(_wkr);
		return WaitForSingleObject(
				   _wkr->native_handle(),
				   timeout.milliseconds<DWORD, INFINITE>()) != WAIT_TIMEOUT;
	}

	virtual waker_i get_waker() {
		if (_wkr) {
			_wkr->reset();
		} else {
			_wkr = std::make_shared<thread_waker>();
		}
		return _wkr;
	}

private:
	duration _yield_dur;
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::win32

#endif
