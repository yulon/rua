#ifndef _RUA_THREAD_WIN32_HPP
#define _RUA_THREAD_WIN32_HPP

#include "../any_word.hpp"
#include "../limits.hpp"
#include "../macros.hpp"
#include "../sched/scheduler.hpp"

#include <windows.h>

#include <cassert>
#include <functional>

namespace rua { namespace win32 {

class thread {
public:
	using native_handle_t = HANDLE;

	using id_t = DWORD;

	////////////////////////////////////////////////////////////////

	static id_t current_id() {
		return GetCurrentThreadId();
	}

	static thread current() {
		return current_id();
	}

	////////////////////////////////////////////////////////////////

	constexpr thread() : _h(nullptr) {}

	explicit thread(std::function<void()> fn) :
		_h(CreateThread(
			nullptr,
			0,
			&_start,
			reinterpret_cast<LPVOID>(new std::function<void()>(std::move(fn))),
			0,
			nullptr)) {}

	thread(id_t id) :
		_h(id ? OpenThread(THREAD_ALL_ACCESS, FALSE, id) : nullptr) {}

	constexpr thread(std::nullptr_t) : thread() {}

	thread(native_handle_t h) : _h(h) {}

	~thread() {
		reset();
	}

	thread(const thread &src) {
		if (!src) {
			_h = nullptr;
			return;
		}
		DuplicateHandle(
			GetCurrentProcess(),
			src._h,
			GetCurrentProcess(),
			&_h,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
	}

	thread(thread &&src) : _h(src._h) {
		if (src) {
			src._h = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(thread)

	id_t id() const {
		return GetThreadId(_h);
	}

	bool operator==(const thread &target) const {
		return id() == target.id();
	}

	bool operator!=(const thread &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		return _h;
	}

	explicit operator bool() const {
		return _h;
	}

	void exit(any_word code = 1) {
		if (!_h) {
			return;
		}
		TerminateThread(_h, code);
		reset();
	}

	any_word wait_for_exit() {
		if (!_h) {
			return 0;
		}
		WaitForSingleObject(_h, INFINITE);
		DWORD exit_code;
		GetExitCodeThread(_h, &exit_code);
		reset();
		return static_cast<int>(exit_code);
	}

	void reset() {
		if (!_h) {
			return;
		}
		CloseHandle(_h);
		_h = nullptr;
	}

private:
	HANDLE _h;

	static DWORD __stdcall _start(LPVOID lpThreadParameter) {
		auto fn_ptr =
			reinterpret_cast<std::function<void()> *>(lpThreadParameter);
		(*fn_ptr)();
		delete fn_ptr;
		return 0;
	}

public:
	class scheduler : public rua::scheduler {
	public:
		static scheduler &instance() {
			static scheduler ds;
			return ds;
		}

		constexpr scheduler(ms yield_dur = 0) : _yield_dur(yield_dur) {}

		virtual ~scheduler() = default;

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

			virtual void reset() {
				ResetEvent(_h);
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
					   sig.to<signaler>()->native_handle(),
					   static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
						   ? nmax<DWORD>()
						   : static_cast<DWORD>(timeout.count())) !=
				   WAIT_TIMEOUT;
		}

	private:
		ms _yield_dur;
	};
};

}} // namespace rua::win32

#endif
