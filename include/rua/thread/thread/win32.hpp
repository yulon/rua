#ifndef _RUA_THREAD_THREAD_WIN32_HPP
#define _RUA_THREAD_THREAD_WIN32_HPP

#include "../this_thread_id/win32.hpp"
#include "../thread_id/win32.hpp"

#include "../../any_word.hpp"
#include "../../dylib/win32.hpp"
#include "../../sched/sys_wait/sync/win32.hpp"

#include <tlhelp32.h>
#include <windows.h>

#include <functional>

namespace rua { namespace win32 {

class thread {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	constexpr thread() : _h(nullptr), _id(0) {}

	explicit thread(std::function<void()> fn) {
		_h = CreateThread(
			nullptr,
			0,
			&_call,
			reinterpret_cast<LPVOID>(new std::function<void()>(std::move(fn))),
			0,
			&_id);
	}

	explicit thread(tid_t id) :
		_h(id ? OpenThread(SYNCHRONIZE, FALSE, id) : nullptr),
		_id(id) {}

	constexpr thread(std::nullptr_t) : thread() {}

	explicit thread(native_handle_t h) : _h(h), _id(0) {}

	~thread() {
		reset();
	}

	thread(const thread &src) : _id(src._id) {
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

	thread(thread &&src) : _h(src._h), _id(src._id) {
		if (src) {
			src._h = nullptr;
			src._id = 0;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(thread)

	tid_t id() {
		if (!_id) {
			_id = _get_id(_h);
		}
		return _id;
	}

	tid_t id() const {
		return _id ? _id : _get_id(_h);
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
		sys_wait(_h);
		DWORD exit_code;
		GetExitCodeThread(_h, &exit_code);
		reset();
		return static_cast<int>(exit_code);
	}

	void reset() {
		if (_h) {
			CloseHandle(_h);
			_h = nullptr;
		}
		if (_id) {
			_id = 0;
		}
	}

private:
	HANDLE _h;
	DWORD _id;

	static DWORD __stdcall _call(LPVOID param) {
		auto fn_ptr = reinterpret_cast<std::function<void()> *>(param);
		(*fn_ptr)();
		delete fn_ptr;
		return 0;
	}

	static DWORD _get_id(HANDLE h) {
		static dylib kernel32("KERNEL32.DLL", false);
		static auto GetThreadId_ptr =
			kernel32.get<decltype(&GetThreadId)>("GetThreadId");

		if (GetThreadId_ptr) {
			return GetThreadId_ptr(h);
		}

		THREADENTRY32 entry;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (snap == INVALID_HANDLE_VALUE) {
			return 0;
		}
		if (Thread32First(snap, &entry)) {
			do {
				HANDLE oh =
					OpenThread(THREAD_ALL_ACCESS, FALSE, entry.th32ThreadID);
				if (oh != NULL) {
					if (oh == h) {
						CloseHandle(oh);
						CloseHandle(snap);
						return entry.th32ThreadID;
					}
					CloseHandle(oh);
				}
			} while (Thread32Next(snap, &entry));
		}
		CloseHandle(snap);
		return 0;
	}
};

}} // namespace rua::win32

#endif
