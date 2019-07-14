#ifndef _RUA_WIN32_THREAD_HPP
#define _RUA_WIN32_THREAD_HPP

#include "../macros.hpp"
#include "../any_word.hpp"
#include "../sched/abstract.hpp"

#include <windows.h>

#include <functional>

namespace rua {
namespace win32 {

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

	explicit thread(std::function<void()> fn) : _h(CreateThread(
		nullptr,
		0,
		[](LPVOID lpThreadParameter)->DWORD {
			auto fn_ptr = reinterpret_cast<std::function<void()> *>(lpThreadParameter);
			(*fn_ptr)();
			delete fn_ptr;
			return 0;
		},
		reinterpret_cast<LPVOID>(new std::function<void()>(std::move(fn))),
		0,
		nullptr
	)) {}

	thread(id_t id) :
		_h(id ? OpenThread(THREAD_ALL_ACCESS, FALSE, id) : nullptr)
	{}

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
			DUPLICATE_SAME_ACCESS
		);
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

	operator bool() const {
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

public:
	class scheduler : public rua::scheduler {
	public:
		static scheduler &instance() {
			static scheduler ds;
			return ds;
		}

		scheduler() = default;

		scheduler(size_t yield_dur) : _yield_dur(yield_dur) {}

		virtual ~scheduler() = default;

		virtual void yield() {
			if (_yield_dur > 1){
				Sleep(_yield_dur);
			}
			for (auto i = 0; i < 3; ++i) {
				if (SwitchToThread()) {
					return;
				}
			}
			Sleep(1);
		}

		virtual void sleep(size_t timeout) {
			Sleep(timeout);
		}

		virtual void lock(typeless_lock_ref &lck) {
			lck.lock();
		}

		class cond_var : public rua::scheduler::cond_var {
		public:
			cond_var() = default;
			virtual ~cond_var() = default;

			virtual void notify() {
				_cv.notify_one();
			}

		private:
			std::condition_variable_any _cv;
			friend scheduler;
		};

		virtual scheduler::cond_var_i make_cond_var() {
			return std::make_shared<cond_var>();
		}

		virtual std::cv_status cond_wait(scheduler::cond_var_i cv, typeless_lock_ref &lck, size_t timeout = nmax<size_t>()) {
			auto dscv = cv.to<cond_var>();
			if (timeout == nmax<size_t>()) {
				dscv->_cv.wait(lck);
				return std::cv_status::no_timeout;
			}
			return dscv->_cv.wait_for(lck, std::chrono::milliseconds(timeout));
		}

	private:
		size_t _yield_dur;
	};
};

}
}

#endif
