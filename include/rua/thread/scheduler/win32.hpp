#ifndef _RUA_THREAD_SCHEDULER_WIN32_HPP
#define _RUA_THREAD_SCHEDULER_WIN32_HPP

#include "../../macros.hpp"
#include "../../sched/scheduler/abstract.hpp"
#include "../../types/util.hpp"

#include <windows.h>

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

class thread_resumer : public resumer {
public:
	using native_handle_t = HANDLE;

	thread_resumer() : _h(CreateEventW(nullptr, false, false, nullptr)) {}

	virtual ~thread_resumer() {
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

	virtual void resume() {
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
		_yield_dur(yield_dur), _rsmr() {}

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

	virtual void sleep(duration timeout) {
		Sleep(timeout.milliseconds<DWORD, INFINITE>());
	}

	virtual bool suspend(duration timeout) {
		assert(_rsmr);

		return WaitForSingleObject(
				   _rsmr->native_handle(),
				   timeout.milliseconds<DWORD, INFINITE>()) != WAIT_TIMEOUT;
	}

	virtual resumer_i get_resumer() {
		if (_rsmr) {
			_rsmr->reset();
		} else {
			_rsmr = std::make_shared<thread_resumer>();
		}
		return _rsmr;
	}

private:
	duration _yield_dur;
	std::shared_ptr<thread_resumer> _rsmr;
};

}} // namespace rua::win32

#endif
