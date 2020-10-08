#ifndef _RUA_PROCESS_WIN32_HPP
#define _RUA_PROCESS_WIN32_HPP

#include "../any_word.hpp"
#include "../bytes.hpp"
#include "../dylib/win32.hpp"
#include "../file/win32.hpp"
#include "../generic_ptr.hpp"
#include "../io.hpp"
#include "../macros.hpp"
#include "../sched/wait/win32.hpp"
#include "../stdio/win32.hpp"
#include "../string/char_enc/base/win32.hpp"
#include "../string/view.hpp"
#include "../sys/info/win32.hpp"
#include "../thread/wait/win32.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>

#include <cassert>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace rua { namespace win32 {

using pid_t = DWORD;

namespace _this_pid {

inline pid_t this_pid() {
	return GetCurrentProcessId();
}

} // namespace _this_pid

using namespace _this_pid;

class process_finder;

class process {
public:
	using native_handle_t = HANDLE;

	static inline process_finder find(string_view name);

	static inline process_finder
	wait_for_found(string_view name, duration interval = 50);

	////////////////////////////////////////////////////////////////

	explicit process(
		const file_path &file,
		const std::vector<std::string> &args = {},
		const file_path &pwd = "",
		bool lazy_start = false,
		sys_stream stdout_w = out(),
		sys_stream stderr_w = err(),
		sys_stream stdin_r = in()) {

		std::wstringstream cmd;
		if (file.str().find(' ') == std::string::npos) {
			cmd << u8_to_w(file.str());
		} else {
			cmd << L"\"" << u8_to_w(file.str()) << L"\"";
		}
		for (auto &arg : args) {
			if (arg.find(' ') == std::string::npos) {
				cmd << L" " << u8_to_w(arg);
			} else {
				cmd << L" \"" << u8_to_w(arg) << L"\"";
			}
		}

		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESTDHANDLES;

		if (stdout_w && !SetHandleInformation(
							stdout_w.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
			_reset();
			return;
		}
		si.hStdOutput = stdout_w.native_handle();

		if (stderr_w && !SetHandleInformation(
							stderr_w.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
			_reset();
			return;
		}
		si.hStdError = stderr_w.native_handle();

		if (stdin_r && !SetHandleInformation(
						   stdin_r.native_handle(),
						   HANDLE_FLAG_INHERIT,
						   HANDLE_FLAG_INHERIT)) {
			_reset();
			return;
		}
		si.hStdInput = stdin_r.native_handle();

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));

		if (!CreateProcessW(
				nullptr,
				const_cast<wchar_t *>(cmd.str().c_str()),
				nullptr,
				nullptr,
				true,
				lazy_start ? CREATE_SUSPENDED : 0,
				nullptr,
				pwd ? u8_to_w(pwd.str()).c_str() : nullptr,
				&si,
				&pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			_reset();
			return;
		}

		_h = pi.hProcess;

		if (lazy_start) {
			_main_td_h = pi.hThread;
		} else {
			CloseHandle(pi.hThread);
			_main_td_h = nullptr;
		}
	}

	explicit process(pid_t id) :
		_h(id ? OpenProcess(PROCESS_ALL_ACCESS, FALSE, id) : nullptr),
		_main_td_h(nullptr) {}

	constexpr process(std::nullptr_t = nullptr) :
		_h(nullptr), _main_td_h(nullptr) {}

	template <
		typename NativeHandle,
		typename = enable_if_t<
			std::is_same<NativeHandle, native_handle_t>::value &&
			!is_null_pointer<NativeHandle>::value>>
	explicit process(NativeHandle h) : _h(h), _main_td_h(nullptr) {}

	~process() {
		reset();
	}

	process(process &&src) : _h(src._h), _main_td_h(src._main_td_h) {
		if (src) {
			src._reset();
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(process)

	pid_t id() const {
		return GetProcessId(_h);
	}

	bool operator==(const process &target) const {
		return id() == target.id();
	}

	bool operator!=(const process &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		return _h;
	}

	operator bool() const {
		return _h;
	}

	void start() {
		if (!_main_td_h) {
			return;
		}
		ResumeThread(_main_td_h);
		CloseHandle(_main_td_h);
		_main_td_h = nullptr;
	}

	int wait_for_exit() {
		if (!_h) {
			return -1;
		}
		start();
		wait(_h);
		DWORD exit_code;
		GetExitCodeProcess(_h, &exit_code);
		reset();
		return static_cast<int>(exit_code);
	}

	void kill() {
		if (!_h) {
			return;
		}
		TerminateProcess(_h, 1);
		_main_td_h = nullptr;
		reset();
	}

	void reset() {
		start();
		if (!_h) {
			return;
		}
		CloseHandle(_h);
		_h = nullptr;
	}

	using native_module_handle_t = HMODULE;

	file_path path(native_module_handle_t mdu = nullptr) const {
		WCHAR pth[MAX_PATH + 1];
		auto pth_sz = GetModuleFileNameExW(_h, mdu, pth, MAX_PATH);
		return w_to_u8(std::wstring(pth, pth_sz));
	}

	size_t memory_usage() const {
		PROCESS_MEMORY_COUNTERS pmc;
		return GetProcessMemoryInfo(_h, &pmc, sizeof(pmc)) ? pmc.WorkingSetSize
														   : 0;
	}

	////////////////////////////////////////////////////////////////

	class memory_block {
	public:
		memory_block() {}

		memory_block(process &p, size_t n) :
			_owner(&p),
			_p(VirtualAllocEx(
				p.native_handle(), nullptr, n, MEM_COMMIT, PAGE_READWRITE)),
			_sz(n) {}

		~memory_block() {
			reset();
		}

		void *data() const {
			return _p;
		}

		size_t size() const {
			return _sz;
		}

		ptrdiff_t write_at(ptrdiff_t pos, bytes_view data) {
			SIZE_T sz;
			if (!WriteProcessMemory(
					_owner->native_handle(),
					reinterpret_cast<void *>(
						reinterpret_cast<uintptr_t>(_p) + pos),
					data.data(),
					data.size(),
					&sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		void detach() {
			_owner = nullptr;
		}

		void reset() {
			if (_owner) {
				VirtualFreeEx(_owner->native_handle(), _p, _sz, MEM_RELEASE);
				_owner = nullptr;
			}
			_p = nullptr;
			_sz = 0;
		}

	private:
		const process *_owner;
		void *_p;
		size_t _sz;
	};

	bytes_view memory_view() const {
		assert(id() == GetCurrentProcessId());

		MODULEINFO mi;
		GetModuleInformation(
			_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return bytes_view(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	memory_block memory_alloc(size_t size) {
		return memory_block(*this, size);
	}

	memory_block memory_alloc(const std::string &str) {
		auto sz = str.length() + 1;
		memory_block data(*this, sz);
		data.write_at(0, bytes_view(str.data(), sz));
		return data;
	}

	memory_block memory_alloc(const std::wstring &wstr) {
		auto sz = (wstr.length() + 1) * sizeof(wchar_t);
		memory_block data(*this, sz);
		data.write_at(0, bytes_view(wstr.data(), sz));
		return data;
	}

	thread make_thread(generic_ptr func, any_word param = nullptr) {
		if (sys_version() >= 6) {
			static auto ntdll = dylib::from_loaded("ntdll.dll");
			static HRESULT(WINAPI * NtCreateThreadEx_ptr)(
				PHANDLE ThreadHandle,
				ACCESS_MASK DesiredAccess,
				std::nullptr_t ObjectAttributes,
				HANDLE ProcessHandle,
				PVOID StartRoutine,
				PVOID StartContext,
				ULONG Flags,
				ULONG_PTR StackZeroBits,
				SIZE_T StackCommit,
				SIZE_T StackReserve,
				PVOID AttributeList) = ntdll["NtCreateThreadEx"];
			if (NtCreateThreadEx_ptr) {
				HANDLE td;
				if (SUCCEEDED(NtCreateThreadEx_ptr(
						&td,
						0x1FFFFF,
						nullptr,
						_h,
						func,
						param,
						0,
						0,
						0,
						0,
						nullptr))) {
					return thread(td);
				}
			}
		}
		return thread(
			CreateRemoteThread(_h, nullptr, 0, func, param, 0, nullptr));
	}

private:
	HANDLE _h, _main_td_h;

	void _reset() {
		_h = nullptr;
		_main_td_h = nullptr;
	}
};

class process_finder : private wandering_iterator {
public:
	process_finder() : _snapshot(INVALID_HANDLE_VALUE) {}

	~process_finder() {
		if (*this) {
			_reset();
		}
	}

	process_finder(process_finder &&src) :
		_snapshot(src._snapshot), _entry(src._entry) {
		if (src) {
			src._snapshot = INVALID_HANDLE_VALUE;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(process_finder)

	operator bool() const {
		return _snapshot != INVALID_HANDLE_VALUE;
	}

	process operator*() const {
		return *this ? process(_entry.th32ProcessID) : process();
	}

	process_finder &operator++() {
		assert(*this);

		while (Process32NextW(_snapshot, &_entry)) {
			if (!_name.size() || _entry.szExeFile == _name) {
				if (_entry.th32ProcessID) {
					return *this;
				}
			}
		}
		_reset();
		return *this;
	}

private:
	std::wstring _name;
	HANDLE _snapshot;
	PROCESSENTRY32W _entry;

	void _reset() {
		CloseHandle(_snapshot);
		_snapshot = INVALID_HANDLE_VALUE;
	}

	process_finder(string_view name) {
		_name = u8_to_w(name);

		_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (_snapshot == INVALID_HANDLE_VALUE) {
			return;
		}

		_entry.dwSize = sizeof(PROCESSENTRY32W);

		if (!Process32FirstW(_snapshot, &_entry)) {
			_reset();
			return;
		}
		do {
			if (!_name.size() || _entry.szExeFile == _name) {
				if (_entry.th32ProcessID) {
					return;
				}
			}
		} while (Process32NextW(_snapshot, &_entry));
		_reset();
	}

	friend process;
};

inline process_finder process::find(string_view name) {
	return process_finder(name);
}

inline process_finder
process::wait_for_found(string_view name, duration interval) {
	process_finder pf;
	for (;;) {
		pf = find(name);
		if (pf) {
			break;
		}
		sleep(interval);
	}
	return pf;
}

namespace _this_process {

inline process this_process() {
	return process(this_pid());
}

} // namespace _this_process

using namespace _this_process;

}} // namespace rua::win32

#endif
