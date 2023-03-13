#ifndef _rua_thread_core_win32_hpp
#define _rua_thread_core_win32_hpp

#include "../id/win32.hpp"

#include "../../dylib/win32.hpp"
#include "../../generic_word.hpp"
#include "../../sync/future.hpp"
#include "../../sync/then.hpp"
#include "../../sys/info/win32.hpp"
#include "../../sys/wait/win32.hpp"
#include "../../util.hpp"

#include <tlhelp32.h>
#include <windows.h>

#include <functional>

namespace rua { namespace win32 {

class thread : private enable_await_operators {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	explicit thread(std::function<void()> fn, size_t stack_size = 0) {
		_h = CreateThread(
			nullptr,
			stack_size,
			&_call,
			reinterpret_cast<LPVOID>(new std::function<void()>(std::move(fn))),
			0,
			&_id);
	}

	explicit thread(tid_t id) :
		_h(id ? OpenThread(_all_access(), false, id) : nullptr), _id(id) {}

	constexpr thread(std::nullptr_t = nullptr) : _h(nullptr), _id(0) {}

	template <
		typename NativeHandle,
		typename = enable_if_t<
			std::is_same<NativeHandle, native_handle_t>::value &&
			!is_null_pointer<NativeHandle>::value>>
	explicit thread(NativeHandle h) : _h(h), _id(0) {}

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

	void exit(generic_word code = 0) {
		if (!_h) {
			return;
		}
		TerminateThread(_h, code);
		reset();
	}

	future<generic_word> RUA_OPERATOR_AWAIT() const {
		auto h = _h;
		return sys_wait(h) >> [h]() -> generic_word {
			DWORD ec;
			GetExitCodeThread(h, &ec);
			return ec;
		};
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
		static dylib kernel32("kernel32.dll");
		static decltype(&GetThreadId) GetThreadId_ptr = kernel32["GetThreadId"];

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

	static DWORD _all_access() {
		if (sys_version() >= 6) {
			return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF;
		}
		return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3FF;
	}
};

namespace _this_thread {

inline thread this_thread() {
	return thread(this_tid());
}

} // namespace _this_thread

using namespace _this_thread;

}} // namespace rua::win32

#endif
