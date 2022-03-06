#ifndef _RUA_THREAD_DOZER_WIN32_HPP
#define _RUA_THREAD_DOZER_WIN32_HPP

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

class thread_waker {
public:
	using native_handle_t = HANDLE;

	thread_waker() : _h(CreateEventW(nullptr, false, false, nullptr)) {}

	~thread_waker() {
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

	void wake() {
		SetEvent(_h);
	}

	void reset() {
		ResetEvent(_h);
	}

private:
	HANDLE _h;
};

class thread_dozer {
public:
	constexpr thread_dozer() : _wkr() {}

	~thread_dozer() = default;

	void sleep(duration timeout) {
		assert(timeout >= 0);

		Sleep(timeout.milliseconds<DWORD, INFINITE>());
	}

	bool doze(duration timeout = duration_max()) {
		assert(_wkr);
		assert(timeout >= 0);

		return WaitForSingleObject(
				   _wkr->native_handle(),
				   timeout.milliseconds<DWORD, INFINITE>()) != WAIT_TIMEOUT;
	}

	std::weak_ptr<thread_waker> get_waker() {
		if (_wkr && _wkr.use_count() == 1) {
			_wkr.reset();
			return _wkr;
		}
		return assign(_wkr, std::make_shared<thread_waker>());
	}

private:
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::win32

#endif
