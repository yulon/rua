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
	static thread_scheduler &instance() {
		static thread_scheduler ds;
		return ds;
	}

	constexpr thread_scheduler(ms yield_dur = 0) : _yield_dur(yield_dur) {}

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

		signaler(native_handle_t h) : _h(h) {}

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

		virtual void signal() {
			SetEvent(_h);
		}

	private:
		HANDLE _h;
	};

	virtual signaler_i make_signaler() {
		return std::make_shared<signaler>();
	}

	virtual bool wait(signaler_i sig, ms timeout = duration_max()) {
		assert(sig.type_is<signaler>());

		return WaitForSingleObject(
				   sig.as<signaler>()->native_handle(),
				   timeout == duration_max()
					   ? INFINITE
					   : (static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
							  ? nmax<DWORD>()
							  : static_cast<DWORD>(timeout.count()))) !=
			   WAIT_TIMEOUT;
	}

private:
	ms _yield_dur;
};

}} // namespace rua::win32

#endif
