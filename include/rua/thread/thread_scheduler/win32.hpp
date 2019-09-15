#ifndef _RUA_THREAD_THREAD_SCHEDULER_WIN32_HPP
#define _RUA_THREAD_THREAD_SCHEDULER_WIN32_HPP

#include "../../limits.hpp"
#include "../../macros.hpp"
#include "../../sched/scheduler.hpp"

#include <windows.h>

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

class thread_scheduler : public rua::scheduler {
public:
	constexpr thread_scheduler(ms yield_dur = 0) :
		_yield_dur(yield_dur),
		_sig() {}

	virtual ~thread_scheduler() = default;

	virtual void yield() {
		if (_yield_dur > 1) {
			Sleep(
				static_cast<int64_t>(nmax<DWORD>()) < _yield_dur.count()
					? nmax<DWORD>()
					: static_cast<DWORD>(_yield_dur.count()));
		}
		for (auto i = 0; i < 3; ++i) {
			if (SwitchToThread()) {
				return;
			}
		}
		Sleep(1);
	}

	virtual void sleep(ms timeout) {
		Sleep(
			static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
				? nmax<DWORD>()
				: static_cast<DWORD>(timeout.count()));
	}

	class signaler : public rua::scheduler::signaler {
	public:
		using native_handle_t = HANDLE;

		signaler() : _h(CreateEventW(nullptr, false, false, nullptr)) {}

		virtual ~signaler() {
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

		virtual void signal() {
			SetEvent(_h);
		}

	private:
		HANDLE _h;
	};

	virtual signaler_i make_signaler() {
		if (!_sig) {
			_sig = std::make_shared<signaler>();
		}
		return _sig;
	}

	virtual bool wait(ms timeout = duration_max()) {
		assert(_sig);
		return WaitForSingleObject(
				   _sig->native_handle(),
				   timeout == duration_max()
					   ? INFINITE
					   : (static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
							  ? nmax<DWORD>()
							  : static_cast<DWORD>(timeout.count()))) !=
			   WAIT_TIMEOUT;
	}

private:
	ms _yield_dur;
	std::shared_ptr<signaler> _sig;
};

}} // namespace rua::win32

#endif
