#ifndef _RUA_THREAD_THREAD_WIN32_HPP
#define _RUA_THREAD_THREAD_WIN32_HPP

#include "../../any_word.hpp"
#include "../thread_id/win32.hpp"

#include <windows.h>

#include <functional>

namespace rua { namespace win32 {

class thread {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	constexpr thread() : _h(nullptr) {}

	explicit thread(std::function<void()> fn) :
		_h(CreateThread(
			nullptr,
			0,
			&_call,
			reinterpret_cast<LPVOID>(new std::function<void()>(std::move(fn))),
			0,
			nullptr)) {}

	explicit thread(thread_id_t id) :
		_h(id ? OpenThread(THREAD_ALL_ACCESS, FALSE, id) : nullptr) {}

	constexpr thread(std::nullptr_t) : thread() {}

	explicit thread(native_handle_t h) : _h(h) {}

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

	thread_id_t id() const {
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

	static DWORD __stdcall _call(LPVOID param) {
		auto fn_ptr = reinterpret_cast<std::function<void()> *>(param);
		(*fn_ptr)();
		delete fn_ptr;
		return 0;
	}
};

}} // namespace rua::win32

#endif
