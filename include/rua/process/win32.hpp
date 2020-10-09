#ifndef _RUA_PROCESS_WIN32_HPP
#define _RUA_PROCESS_WIN32_HPP

#include "../any_word.hpp"
#include "../bytes.hpp"
#include "../dylib/win32.hpp"
#include "../file/win32.hpp"
#include "../generic_ptr.hpp"
#include "../io.hpp"
#include "../macros.hpp"
#include "../memory.hpp"
#include "../sched/wait/win32.hpp"
#include "../stdio/win32.hpp"
#include "../string/char_enc/base/win32.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"
#include "../sys/info/win32.hpp"
#include "../thread/wait/win32.hpp"
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
		const std::vector<string_view> &args = {},
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
			if (arg.find(' ') == string_view::npos) {
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
		_h(id ? OpenProcess(PROCESS_ALL_ACCESS, false, id) : nullptr),
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

	bool start_with_dylibs(const std::vector<string_view> &names) {
		if (!_main_td_h) {
			return false;
		}

		bytes names_b;
		for (auto &name : names) {
			names_b += u8_to_w(name);
			names_b += L'\0';
		}
		names_b += L'\0';

		CONTEXT main_td_ctx;
		main_td_ctx.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(_main_td_h, &main_td_ctx)) {
			return false;
		}

		_load_dll_ctx ctx;

#if RUA_X86 == 64

		ctx.RtlUserThreadStart = generic_ptr(main_td_ctx.Rip);
		ctx.td_start = generic_ptr(main_td_ctx.Rcx);
		ctx.td_param = generic_ptr(main_td_ctx.Rdx);

#elif RUA_X86 == 32

		ctx.RtlUserThreadStart = generic_ptr(main_td_ctx.Eip);
		ctx.td_start = generic_ptr(main_td_ctx.Eax);
		ctx.td_param = generic_ptr(main_td_ctx.Edx);

#endif

		auto data = _make_load_dll_data(names_b, ctx);
		if (!data) {
			return false;
		}

#if RUA_X86 == 64

		main_td_ctx.Rip = data.dll_loader.data().uintptr();
		main_td_ctx.Rcx = data.ctx.data().uintptr();

#elif RUA_X86 == 32

		main_td_ctx.Eip = data.dll_loader.data().uintptr();
		auto stk = memory_ref(generic_ptr(main_td_ctx.Esp), 2 * sizeof(void *));
		if (stk.write_at(sizeof(void *), data.ctx.data().uintptr()) <= 0) {
			return false;
		}

#endif

		if (!SetThreadContext(_main_td_h, &main_td_ctx)) {
			return false;
		}

		data.ctx.detach();
		data.names.detach();
		data.dll_loader.detach();

		start();
		return true;
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

	using native_module_handle_t = HMODULE;

	file_path path(native_module_handle_t mdu = nullptr) const {
		WCHAR pth[MAX_PATH + 1];
		auto pth_sz = GetModuleFileNameExW(_h, mdu, pth, MAX_PATH);
		return w_to_u8(std::wstring(pth, pth_sz));
	}

	bytes_view image() const {
		assert(id() == GetCurrentProcessId());

		MODULEINFO mi;
		GetModuleInformation(
			_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return bytes_view(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	size_t memory_usage() const {
		PROCESS_MEMORY_COUNTERS pmc;
		return GetProcessMemoryInfo(_h, &pmc, sizeof(pmc)) ? pmc.WorkingSetSize
														   : 0;
	}

	void reset() {
		start();
		if (!_h) {
			return;
		}
		CloseHandle(_h);
		_h = nullptr;
	}

	////////////////////////////////////////////////////////////////

	class memory_block {
	public:
		constexpr memory_block() :
			_owner(nullptr), _p(nullptr), _sz(0), _need_free(false) {}

		~memory_block() {
			reset();
		}

		operator bool() const {
			return _p;
		}

		generic_ptr data() const {
			return _p;
		}

		size_t size() const {
			return _sz;
		}

		ptrdiff_t read_at(ptrdiff_t pos, bytes_ref data) {
			SIZE_T sz;
			if (!ReadProcessMemory(
					_owner->native_handle(),
					_p + pos,
					data.data(),
					data.size(),
					&sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		ptrdiff_t write_at(ptrdiff_t pos, bytes_view data) {
			SIZE_T sz;
			if (!WriteProcessMemory(
					_owner->native_handle(),
					_p + pos,
					data.data(),
					data.size(),
					&sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		void detach() {
			_need_free = false;
		}

		void reset() {
			if (_need_free) {
				VirtualFreeEx(_owner->native_handle(), _p, _sz, MEM_RELEASE);
				_need_free = false;
			}
			_owner = nullptr;
			_p = nullptr;
			_sz = 0;
		}

	private:
		const process *_owner;
		generic_ptr _p;
		size_t _sz;
		bool _need_free;

		memory_block(process &p, generic_ptr data, size_t n, bool need_free) :
			_owner(&p), _p(data), _sz(n), _need_free(need_free) {}

		friend process;
	};

	memory_block memory_ref(generic_ptr data, size_t size) {
		return memory_block(*this, data, size, false);
	}

	memory_block memory_alloc(size_t size) {
		return memory_block(
			*this,
			VirtualAllocEx(_h, nullptr, size, MEM_COMMIT, PAGE_READWRITE),
			size,
			true);
	}

	template <typename T>
	enable_if_t<
		std::is_constructible<bytes_view, T &&>::value &&
			!std::is_convertible<T &&, string_view>::value &&
			!std::is_convertible<T &&, wstring_view>::value,
		memory_block>
	memory_alloc(T &&val) {
		bytes_view bv(val);
		auto data = memory_alloc(bv.size());
		data.write_at(0, bv);
		return data;
	}

	memory_block memory_alloc(const std::string &str) {
		auto sz = str.length() + 1;
		auto data = memory_alloc(sz);
		data.write_at(0, bytes_view(str.data(), sz));
		return data;
	}

	memory_block memory_alloc(const std::wstring &wstr) {
		auto sz = (wstr.length() + 1) * sizeof(wchar_t);
		auto data = memory_alloc(sz);
		data.write_at(0, bytes_view(wstr.data(), sz));
		return data;
	}

	thread make_thread(generic_ptr func, generic_ptr param = nullptr) {
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

	HANDLE load_dylib(string_view name) {
		bytes names(((name.length() + 1) + 1) * sizeof(wchar_t));
		names.resize(0);
		names += u8_to_w(name);
		names += L'\0';
		names += L'\0';

		_load_dll_ctx ctx;
		ctx.RtlUserThreadStart = nullptr;

		auto data = _make_load_dll_data(names, ctx);
		if (!data) {
			return nullptr;
		}

		return make_thread(data.dll_loader.data(), data.ctx.data())
			.wait_for_exit();
	}

private:
	HANDLE _h, _main_td_h;

	void _reset() {
		_h = nullptr;
		_main_td_h = nullptr;
	}

	struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR Buffer;
	};

	struct _load_dll_ctx {
		HRESULT(WINAPI *RtlInitUnicodeString)(_UNICODE_STRING *, PCWSTR);
		HRESULT(WINAPI *LdrLoadDll)(PWCHAR, ULONG, _UNICODE_STRING *, PHANDLE);
		const wchar_t *names;

		void(RUA_REGPARM(3) * RtlUserThreadStart)(PTHREAD_START_ROUTINE, PVOID);
		PTHREAD_START_ROUTINE td_start;
		PVOID td_param;
	};

	static HANDLE WINAPI _dll_loader(_load_dll_ctx *ctx) {
		HANDLE h = nullptr;

		auto names = ctx->names;

		for (auto it = names;;) {
			if (*it != 0) {
				++it;
				continue;
			}
			auto sz = it - names;
			if (!sz) {
				break;
			}
			_UNICODE_STRING us;
			ctx->RtlInitUnicodeString(&us, names);
			ctx->LdrLoadDll(nullptr, 0, &us, &h);
			names = ++it;
		}

		if (ctx->RtlUserThreadStart) {
#if defined(_MSC_VER) && RUA_X86 == 32
			auto RtlUserThreadStart = ctx->RtlUserThreadStart;
			auto td_start = ctx->td_start;
			auto td_param = ctx->td_param;
			__asm {
				mov eax, td_start
				mov edx, td_param
				call RtlUserThreadStart
			}
#else
			ctx->RtlUserThreadStart(ctx->td_start, ctx->td_param);
#endif
		}

		return h;
	}

	struct _load_dll_data {
		memory_block names, dll_loader, ctx;

		operator bool() const {
			return names && dll_loader && ctx;
		}
	};

	_load_dll_data _make_load_dll_data(bytes_view names, _load_dll_ctx &ctx) {
		_load_dll_data data;

		data.names = memory_alloc(names);
		if (!data.names) {
			return data;
		}

		ctx.names = data.names.data();

		static auto ntdll = dylib::from_loaded("ntdll.dll");
		static auto RtlInitUnicodeString_ptr = ntdll["RtlInitUnicodeString"];
		static auto LdrLoadDll_ptr = ntdll["LdrLoadDll"];

		ctx.RtlInitUnicodeString = RtlInitUnicodeString_ptr;
		ctx.LdrLoadDll = LdrLoadDll_ptr;

		data.ctx = memory_alloc(ctx);
		if (!data.ctx) {
			return data;
		}

		data.dll_loader = memory_alloc(bytes_view(&_dll_loader, 512));
		if (!data.dll_loader) {
			return data;
		}

		DWORD old_mode;
		if (!VirtualProtectEx(
				_h,
				data.dll_loader.data(),
				data.dll_loader.size(),
				PAGE_EXECUTE_READWRITE,
				&old_mode)) {
			return data;
		}

		return data;
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
