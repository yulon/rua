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
#include "../sched/await/win32.hpp"
#include "../stdio/win32.hpp"
#include "../string/char_enc/base/win32.hpp"
#include "../string/char_set.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"
#include "../sys/info/win32.hpp"
#include "../sys/stream/win32.hpp"
#include "../thread/wait/win32.hpp"
#include "../types/util.hpp"

#include <psapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <winternl.h>

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <list>
#include <memory>
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

	template <RUA_STRING_RANGE(StrList)>
	explicit process(
		const file_path &file,
		const StrList &args = {},
		const file_path &wd = "",
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
		RUA_RANGE_FOR(string_view arg, args, {
			if (arg.find(' ') == string_view::npos) {
				cmd << L" " << u8_to_w(arg);
			} else {
				cmd << L" \"" << u8_to_w(arg) << L"\"";
			}
		})

		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESTDHANDLES;

		if (stdout_w && !SetHandleInformation(
							stdout_w.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
			_h = nullptr;
			return;
		}
		si.hStdOutput = stdout_w.native_handle();

		if (stderr_w && !SetHandleInformation(
							stderr_w.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
			_h = nullptr;
			return;
		}
		si.hStdError = stderr_w.native_handle();

		if (stdin_r && !SetHandleInformation(
						   stdin_r.native_handle(),
						   HANDLE_FLAG_INHERIT,
						   HANDLE_FLAG_INHERIT)) {
			_h = nullptr;
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
				wd ? u8_to_w(wd.str()).c_str() : nullptr,
				&si,
				&pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			_h = nullptr;
			return;
		}

		_h = pi.hProcess;

		if (!lazy_start) {
			CloseHandle(pi.hThread);
			return;
		}
		_ext.reset(new _ext_t);
		_ext->main_td = thread(pi.hThread);
	}

	explicit process(pid_t id) :
		_h(id ? OpenProcess(_all_access(), false, id) : nullptr),
		_ext(nullptr) {}

	constexpr process(std::nullptr_t = nullptr) : _h(nullptr), _ext(nullptr) {}

	template <
		typename NativeHandle,
		typename = enable_if_t<
			std::is_same<NativeHandle, native_handle_t>::value &&
			!is_null_pointer<NativeHandle>::value>>
	explicit process(NativeHandle h) : _h(h), _ext(nullptr) {}

	~process() {
		reset();
	}

	process(process &&src) : _h(src._h), _ext(std::move(src._ext)) {
		if (src) {
			src._h = nullptr;
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
		if (!_ext || !_ext->main_td) {
			return;
		}
		if (_ext->lazy_load_dlls.size()) {
			_start_with_load_dlls();
		}
		ResumeThread(_ext->main_td.native_handle());
		_ext->main_td.reset();
	}

	int wait_for_exit() {
		if (!_h) {
			return -1;
		}
		start();
		await(_h);
		DWORD exit_code;
		GetExitCodeProcess(_h, &exit_code);
		_reset();
		return static_cast<int>(exit_code);
	}

	bool kill() {
		if (!_h) {
			return true;
		}
		if (!TerminateProcess(_h, 0)) {
			return false;
		}
		_reset();
		return true;
	}

	bool is_suspended() const {
		auto nt_query_system_information = _ntdll().nt_query_system_information;
		if (!nt_query_system_information) {
			return false;
		}

		DWORD si_buf_sz = 0;

		nt_query_system_information(5, 0, 0, &si_buf_sz);
		if (!si_buf_sz) {
			return false;
		}

		bytes si_buf(si_buf_sz + 0x10000);

		if (nt_query_system_information(
				5, si_buf.data(), si_buf.size(), &si_buf_sz) < 0) {
			return false;
		}

		auto si_data = si_buf(0, si_buf_sz);

		auto pid = id();
		for (;;) {
			auto &spi = si_data.as<SYSTEM_PROCESS_INFORMATION>();
			if (static_cast<pid_t>(
					reinterpret_cast<uintptr_t>(spi.UniqueProcessId)) != pid) {
				if (!spi.NextEntryOffset) {
					return false;
				}
				si_data = si_data(spi.NextEntryOffset);
				continue;
			}
			si_data = si_data(sizeof(SYSTEM_PROCESS_INFORMATION));
			for (size_t i = 0; i < spi.NumberOfThreads; ++i) {
				auto &sti = si_data.aligned_as<SYSTEM_THREAD_INFORMATION>(i);
				if (sti.ThreadState != 5 || sti.WaitReason != 5) {
					return false;
				}
			}
			return true;
		}

		return false;
	}

	bool suspend() {
		if (is_suspended()) {
			return true;
		}
		auto nt_suspend_process = _ntdll().nt_suspend_process;
		if (!nt_suspend_process) {
			return false;
		}
		return nt_suspend_process(_h) >= 0;
	}

	bool resume() {
		auto nt_resume_process = _ntdll().nt_resume_process;
		if (!nt_resume_process) {
			return false;
		}
		return nt_resume_process(_h) >= 0;
	}

	using native_module_handle_t = HMODULE;

	file_path path(native_module_handle_t mdu = nullptr) const {
		auto get_module_file_name_ex_w = _psapi().get_module_file_name_ex_w;
		if (!get_module_file_name_ex_w) {
			return "";
		}
		WCHAR pth[MAX_PATH];
		auto pth_sz = get_module_file_name_ex_w(_h, mdu, pth, MAX_PATH);
		if (!pth_sz) {
			return "";
		}
		return w_to_u8({pth, pth_sz});
	}

	std::string name(native_module_handle_t mdu = nullptr) const {
		auto get_module_base_name_w = _psapi().get_module_base_name_w;
		if (!get_module_base_name_w) {
			return path().back();
		}
		WCHAR n[MAX_PATH];
		auto n_sz = get_module_base_name_w(_h, mdu, n, MAX_PATH);
		if (!n_sz) {
			return path().back();
		}
		return w_to_u8({n, n_sz});
	}

	std::vector<std::string> args() const {
		std::vector<std::string> params;

		int argc;
		LPWSTR *argv;

		if (id() == this_pid()) {
			argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		} else {
			auto nt_query_information_process =
				_ntdll().nt_query_information_process;
			if (!nt_query_information_process) {
				return params;
			}

			PROCESS_BASIC_INFORMATION pbi;
			if (nt_query_information_process(
					_h, 0, &pbi, sizeof(pbi), nullptr) < 0) {
				return params;
			}

			PEB peb;
			if (memory_read(pbi.PebBaseAddress, as_bytes(peb)) != sizeof(peb)) {
				return params;
			}
			RTL_USER_PROCESS_PARAMETERS peb_params;
			if (memory_read(peb.ProcessParameters, as_bytes(peb_params)) !=
				sizeof(peb_params)) {
				return params;
			}
			std::wstring cmd_w(peb_params.CommandLine.Length, 0);
			auto cmd_w_b = as_bytes(cmd_w);
			if (memory_read(peb_params.CommandLine.Buffer, cmd_w_b) !=
				static_cast<ptrdiff_t>(cmd_w_b.size())) {
				return params;
			}

			argv = CommandLineToArgvW(cmd_w.c_str(), &argc);
		}

		if (!argv) {
			return params;
		}

		if (argc > 1) {
			params.reserve(argc - 1);
			for (int i = 1; i < argc; ++i) {
				params.emplace_back(w_to_u8(argv[i]));
			}
		}

		LocalFree(argv);

		return params;
	}

	size_t memory_usage() const {
		auto get_process_memoryinfo = _psapi().get_process_memoryinfo;
		if (!get_process_memoryinfo) {
			return 0;
		}
		PROCESS_MEMORY_COUNTERS pmc;
		return get_process_memoryinfo(_h, &pmc, sizeof(pmc))
				   ? pmc.WorkingSetSize
				   : 0;
	}

	ptrdiff_t memory_read(generic_ptr ptr, bytes_ref data) const {
		SIZE_T sz;
		if (!ReadProcessMemory(_h, ptr, data.data(), data.size(), &sz)) {
			return -1;
		}
		return static_cast<ptrdiff_t>(sz);
	}

	ptrdiff_t memory_write(generic_ptr ptr, bytes_view data) {
		SIZE_T sz;
		if (!WriteProcessMemory(_h, ptr, data.data(), data.size(), &sz)) {
			return -1;
		}
		return static_cast<ptrdiff_t>(sz);
	}

	class memory_block {
	public:
		constexpr memory_block() :
			_h(nullptr), _p(nullptr), _sz(0), _need_free(false) {}

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
					_h, _p + pos, data.data(), data.size(), &sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		ptrdiff_t write_at(ptrdiff_t pos, bytes_view data) {
			SIZE_T sz;
			if (!WriteProcessMemory(
					_h, _p + pos, data.data(), data.size(), &sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		void detach() {
			_need_free = false;
		}

		void reset() {
			if (_need_free) {
				VirtualFreeEx(_h, _p, _sz, MEM_RELEASE);
				_need_free = false;
			}
			_h = nullptr;
			_p = nullptr;
			_sz = 0;
		}

	private:
		native_handle_t _h;
		generic_ptr _p;
		size_t _sz;
		bool _need_free;

		memory_block(
			native_handle_t h, generic_ptr data, size_t n, bool need_free) :
			_h(h), _p(data), _sz(n), _need_free(need_free) {}

		friend process;
	};

	memory_block memory_ref(generic_ptr data, size_t size) const {
		return memory_block(_h, data, size, false);
	}

	memory_block memory_image(native_module_handle_t mdu = nullptr) const {
		auto get_module_information = _psapi().get_module_information;
		if (!get_module_information) {
			return {};
		}
		MODULEINFO mi;
		get_module_information(
			_h, mdu ? mdu : GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return memory_ref(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	memory_block memory_alloc(size_t size, bool is_executable = true) {
		return memory_block(
			_h,
			VirtualAllocEx(
				_h,
				nullptr,
				size,
				MEM_COMMIT,
				is_executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE),
			size,
			true);
	}

	memory_block memory_alloc(bytes_view bv, bool is_executable = true) {
		auto data = memory_alloc(bv.size(), is_executable);
		data.write_at(0, bv);
		return data;
	}

	memory_block memory_alloc(const std::string &str) {
		auto sz = str.length() + 1;
		auto data = memory_alloc(sz);
		data.write_at(0, as_bytes(str.data(), sz));
		return data;
	}

	memory_block memory_alloc(const std::wstring &wstr) {
		auto sz = (wstr.length() + 1) * sizeof(wchar_t);
		auto data = memory_alloc(sz);
		data.write_at(0, as_bytes(wstr.data(), sz));
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

	native_module_handle_t load_dylib(std::string name) {
		if (_ext && _ext->main_td) {
			_ext->lazy_load_dlls.emplace_back(std::move(name));
			return reinterpret_cast<native_module_handle_t>(
				INVALID_HANDLE_VALUE);
		}

		_load_dll_ctx ctx;
		ctx.RtlUserThreadStart = nullptr;

		auto data = _make_load_dll_data(
			std::array<std::string, 1>{std::move(name)}, ctx);
		if (!data) {
			return nullptr;
		}

		return make_thread(_ext->dll_loader.data(), data.ctx.data())
			.wait_for_exit();
	}

	void reset() {
		if (!_h) {
			return;
		}
		start();
		_reset();
	}

private:
	HANDLE _h;

	struct _ext_t {
		thread main_td;
		memory_block dll_loader;
		std::list<std::string> lazy_load_dlls;
	};

	std::unique_ptr<_ext_t> _ext;

	void _reset() {
		assert(_h);

		_ext.reset();
		CloseHandle(_h);
		_h = nullptr;
	}

	static DWORD _all_access() {
		if (sys_version() >= 6) {
			return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF;
		}
		return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF;
	}

	struct _process_extended_basic_information {
		SIZE_T Size;
		PROCESS_BASIC_INFORMATION BasicInfo;
		union {
			ULONG Flags;
			struct {
				ULONG IsProtectedProcess : 1;
				ULONG IsWow64Process : 1;
				ULONG IsProcessDeleting : 1;
				ULONG IsCrossSessionCreate : 1;
				ULONG IsFrozen : 1;
				ULONG IsBackground : 1;
				ULONG IsStronglyNamed : 1;
				ULONG IsSecureProcess : 1;
				ULONG IsSubsystemProcess : 1;
				ULONG SpareBits : 23;
			};
		};
	};

	struct _ntdll_t {
		NTSTATUS(WINAPI *nt_suspend_process)(HANDLE);
		NTSTATUS(WINAPI *nt_resume_process)(HANDLE);
		NTSTATUS(WINAPI *nt_query_system_information)
		(int SystemInformationClass,
		 PVOID SystemInformation,
		 ULONG SystemInformationLength,
		 PULONG ReturnLength);
		NTSTATUS(WINAPI *nt_query_information_process)
		(HANDLE ProcessHandle,
		 int ProcessInformationClass,
		 PVOID ProcessInformation,
		 ULONG ProcessInformationLength,
		 PULONG ReturnLength);
	};

	static const _ntdll_t &_ntdll() {
		static auto ntdll = dylib::from_loaded("ntdll.dll");
		static const _ntdll_t inst{
			ntdll["NtSuspendProcess"],
			ntdll["NtResumeProcess"],
			ntdll["NtQuerySystemInformation"],
			ntdll["NtQueryInformationProcess"]};
		return inst;
	}

	static generic_ptr _load_psapi(string_view name) {
		static auto kernel32 = dylib::from_loaded("kernel32.dll");
		auto fp = kernel32[str_join("K32", name)];
		if (fp) {
			return fp;
		}
		static dylib psapi("psapi.dll");
		return psapi[name];
	}

	struct _psapi_t {
		decltype(&GetModuleFileNameExW) get_module_file_name_ex_w;
		decltype(&GetModuleBaseNameW) get_module_base_name_w;
		decltype(&GetModuleInformation) get_module_information;
		decltype(&GetProcessMemoryInfo) get_process_memoryinfo;
	};

	static const _psapi_t &_psapi() {
		static const _psapi_t inst{
			_load_psapi("GetModuleFileNameExW"),
			_load_psapi("GetModuleBaseNameW"),
			_load_psapi("GetModuleInformation"),
			_load_psapi("GetProcessMemoryInfo")};
		return inst;
	}

	struct _UNICODE_STRING;

	struct _load_dll_ctx {
		HRESULT(WINAPI *RtlInitUnicodeString)(_UNICODE_STRING *, PCWSTR);
		HRESULT(WINAPI *LdrLoadDll)
		(PWCHAR, ULONG, _UNICODE_STRING *, HMODULE *);
		const wchar_t *names;

		void(RUA_REGPARM(3) * RtlUserThreadStart)(PTHREAD_START_ROUTINE, PVOID);
		PTHREAD_START_ROUTINE td_start;
		PVOID td_param;
	};

#ifdef RUA_PROTOTYPE
	static HMODULE WINAPI _dll_loader(_load_dll_ctx *ctx) {
		HMODULE h = nullptr;

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
#endif

	static bytes_view _dll_loader_code() {
		// clang-format off
		static const uchar code[] {
#if RUA_X86 == 64
			0x55,                                                  // push   %rbp
			0x57,                                                  // push   %rdi
			0x56,                                                  // push   %rsi
			0x53,                                                  // push   %rbx
			0x48, 0x83, 0xec, 0x48,                                // sub    $0x48,%rsp
			0x48, 0x8b, 0x51, 0x10,                                // mov    0x10(%rcx),%rdx
			0x48, 0x8d, 0x5a, 0x02,                                // lea    0x2(%rdx),%rbx
			0x48, 0x89, 0xce,                                      // mov    %rcx,%rsi
			0x48, 0x8d, 0x7c, 0x24, 0x30,                          // lea    0x30(%rsp),%rdi
			0x48, 0x8d, 0x6c, 0x24, 0x28,                          // lea    0x28(%rsp),%rbp
			0x48, 0xc7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x00,  // movq   $0x0,0x28(%rsp)
			0xeb, 0x21,                                            // jmp    49 <_dll_loader+0x49>
			0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,        // nopl   0x0(%rax,%rax,1)
			// 30:
			0x48, 0x89, 0xf9,                                      // mov    %rdi,%rcx
			0xff, 0x16,                                            // callq  *(%rsi)
			0x31, 0xd2,                                            // xor    %edx,%edx
			0x49, 0x89, 0xe9,                                      // mov    %rbp,%r9
			0x49, 0x89, 0xf8,                                      // mov    %rdi,%r8
			0x31, 0xc9,                                            // xor    %ecx,%ecx
			0xff, 0x56, 0x08,                                      // callq  *0x8(%rsi)
			0x48, 0x89, 0xda,                                      // mov    %rbx,%rdx
			// 45:
			0x48, 0x83, 0xc3, 0x02,                                // add    $0x2,%rbx
			// 49:
			0x66, 0x83, 0x7b, 0xfe, 0x00,                          // cmpw   $0x0,-0x2(%rbx)
			0x75, 0xf5,                                            // jne    45 <_dll_loader+0x45>
			0x48, 0x8d, 0x43, 0xfe,                                // lea    -0x2(%rbx),%rax
			0x48, 0x39, 0xc2,                                      // cmp    %rax,%rdx
			0x75, 0xd7,                                            // jne    30 <_dll_loader+0x30>
			0x48, 0x8b, 0x46, 0x18,                                // mov    0x18(%rsi),%rax
			0x48, 0x85, 0xc0,                                      // test   %rax,%rax
			0x74, 0x0a,                                            // je     6c <_dll_loader+0x6c>
			0x48, 0x8b, 0x56, 0x28,                                // mov    0x28(%rsi),%rdx
			0x48, 0x8b, 0x4e, 0x20,                                // mov    0x20(%rsi),%rcx
			0xff, 0xd0,                                            // callq  *%rax
			// 6c:
			0x48, 0x8b, 0x44, 0x24, 0x28,                          // mov    0x28(%rsp),%rax
			0x48, 0x83, 0xc4, 0x48,                                // add    $0x48,%rsp
			0x5b,                                                  // pop    %rbx
			0x5e,                                                  // pop    %rsi
			0x5f,                                                  // pop    %rdi
			0x5d,                                                  // pop    %rbp
			0xc3                                                   // retq
#elif RUA_X86 == 32
			0x55,                                                        // push   %ebp
			0x53,                                                        // push   %ebx
			0x57,                                                        // push   %edi
			0x56,                                                        // push   %esi
			0x83, 0xec, 0x0c,                                            // sub    $0xc,%esp
			0x8b, 0x74, 0x24, 0x20,                                      // mov    0x20(%esp),%esi
			0xc7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,                    // movl   $0x0,(%esp)
			0x8d, 0x7c, 0x24, 0x04,                                      // lea    0x4(%esp),%edi
			0x89, 0xe3,                                                  // mov    %esp,%ebx
			0x8b, 0x46, 0x08,                                            // mov    0x8(%esi),%eax
			0x89, 0xc5,                                                  // mov    %eax,%ebp
			0x0f, 0x1f, 0x00,                                            // nopl   (%eax)
			// 20:
			0xb9, 0x02, 0x00, 0x00, 0x00,                                // mov    $0x2,%ecx
			0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,  // nopw   %cs:0x0(%eax,%eax,1)
			0x90,                                                        // nop
			// 30:
			0x83, 0xc1, 0xfe,                                            // add    $0xfffffffe,%ecx
			0x66, 0x83, 0x7d, 0x00, 0x00,                                // cmpw   $0x0,0x0(%ebp)
			0x8d, 0x6d, 0x02,                                            // lea    0x2(%ebp),%ebp
			0x75, 0xf3,                                                  // jne    30 <_dll_loader@4+0x30>
			0x85, 0xc9,                                                  // test   %ecx,%ecx
			0x74, 0x11,                                                  // je     52 <_dll_loader@4+0x52>
			0x50,                                                        // push   %eax
			0x57,                                                        // push   %edi
			0xff, 0x16,                                                  // call   *(%esi)
			0x53,                                                        // push   %ebx
			0x57,                                                        // push   %edi
			0x6a, 0x00,                                                  // push   $0x0
			0x6a, 0x00,                                                  // push   $0x0
			0xff, 0x56, 0x04,                                            // call   *0x4(%esi)
			0x89, 0xe8,                                                  // mov    %ebp,%eax
			0xeb, 0xce,                                                  // jmp    20 <_dll_loader@4+0x20>
			// 52:
			0x8b, 0x4e, 0x0c,                                            // mov    0xc(%esi),%ecx
			0x85, 0xc9,                                                  // test   %ecx,%ecx
			0x74, 0x08,                                                  // je     61 <_dll_loader@4+0x61>
			0x8b, 0x46, 0x10,                                            // mov    0x10(%esi),%eax
			0x8b, 0x56, 0x14,                                            // mov    0x14(%esi),%edx
			0xff, 0xd1,                                                  // call   *%ecx
			// 61:
			0x8b, 0x04, 0x24,                                            // mov    (%esp),%eax
			0x83, 0xc4, 0x0c,                                            // add    $0xc,%esp
			0x5e,                                                        // pop    %esi
			0x5f,                                                        // pop    %edi
			0x5b,                                                        // pop    %ebx
			0x5d,                                                        // pop    %ebp
			0xc2, 0x04, 0x00                                             // ret    $0x4
#endif
		};
		// clang-format on
		return bytes_view(code);
	}

	bool _make_dll_loader() {
		if (!_ext) {
			_ext.reset(new _ext_t);
		}
		if (_ext->dll_loader) {
			return true;
		}
		_ext->dll_loader = memory_alloc(_dll_loader_code(), true);
		return _ext->dll_loader;
	}

	struct _load_dll_data {
		memory_block names, ctx;

		operator bool() const {
			return names && ctx;
		}
	};

	template <typename NameList>
	_load_dll_data _make_load_dll_data(NameList &&names, _load_dll_ctx &ctx) {
		_load_dll_data data;

		if (!_make_dll_loader()) {
			return data;
		}

		bytes names_buf;
		for (auto &name : names) {
			names_buf += as_bytes(u8_to_w(name));
			names_buf += as_bytes(L'\0');
		}
		names_buf += as_bytes(L'\0');

		data.names = memory_alloc(names_buf);
		if (!data.names) {
			return data;
		}

		ctx.names = data.names.data();

		static auto ntdll = dylib::from_loaded("ntdll.dll");
		static auto RtlInitUnicodeString_ptr = ntdll["RtlInitUnicodeString"];
		static auto LdrLoadDll_ptr = ntdll["LdrLoadDll"];

		ctx.RtlInitUnicodeString = RtlInitUnicodeString_ptr;
		ctx.LdrLoadDll = LdrLoadDll_ptr;

		data.ctx = memory_alloc(as_bytes(ctx));
		if (!data.ctx) {
			return data;
		}

		return data;
	}

	bool _start_with_load_dlls() {
		assert(_ext);

		if (_ext->lazy_load_dlls.empty()) {
			return true;
		}

		CONTEXT main_td_ctx;
		main_td_ctx.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(_ext->main_td.native_handle(), &main_td_ctx)) {
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

		auto data = _make_load_dll_data(_ext->lazy_load_dlls, ctx);
		if (!data) {
			return false;
		}

#if RUA_X86 == 64
		main_td_ctx.Rip = _ext->dll_loader.data().uintptr();
		main_td_ctx.Rcx = data.ctx.data().uintptr();
#elif RUA_X86 == 32
		main_td_ctx.Eip = _ext->dll_loader.data().uintptr();
		main_td_ctx.Esp -= 2 * sizeof(uintptr_t);
		auto stk =
			memory_ref(generic_ptr(main_td_ctx.Esp), 2 * sizeof(uintptr_t));
		if (stk.write_at(
				sizeof(uintptr_t), as_bytes(data.ctx.data().uintptr())) <= 0) {
			return false;
		}
#endif

		if (!SetThreadContext(_ext->main_td.native_handle(), &main_td_ctx)) {
			return false;
		}

		data.ctx.detach();
		data.names.detach();
		_ext->dll_loader.detach();

		return true;
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
	return process(GetCurrentProcess());
}

} // namespace _this_process

using namespace _this_process;

namespace _process_privileges {

inline bool this_process_is_privileged() {
	SID_IDENTIFIER_AUTHORITY sna = SECURITY_NT_AUTHORITY;
	PSID admin_group = nullptr;
	auto r = AllocateAndInitializeSid(
		&sna,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,
		0,
		0,
		0,
		0,
		0,
		&admin_group);
	if (r) {
		BOOL is_member = FALSE;
		r = CheckTokenMembership(nullptr, admin_group, &is_member);
		if (r) {
			r = is_member;
		}
	}
	if (admin_group) {
		FreeSid(admin_group);
		admin_group = nullptr;
	}
	return r;
}

template <RUA_STRING_RANGE(StrList)>
inline int execute_with_highest_privileges(
	const file_path &file, const StrList &args = {}, const file_path &wd = "") {

	SHELLEXECUTEINFOW sei;
	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.lpVerb = L"runas";
	sei.nShow = SW_NORMAL;

	std::wstring path_w;
	path_w = u8_to_w(file.str());
	sei.lpFile = path_w.c_str();

	std::wstringstream args_ss_w;
	RUA_RANGE_FOR(string_view arg, args, {
		if (arg.find(' ') == string_view::npos) {
			args_ss_w << L" " << u8_to_w(arg);
		} else {
			args_ss_w << L" \"" << u8_to_w(arg) << L"\"";
		}
	})
	auto args_w = args_ss_w.str();
	if (args_w.length()) {
		sei.lpParameters = args_w.c_str();
	}

	auto wd_w = u8_to_w(wd.str());
	sei.lpDirectory = wd_w.c_str();

	if (ShellExecuteExW(&sei)) {
		return 0;
	}
	return static_cast<int>(GetLastError());
}

inline void restart_as_elevate_privileges() {
	if (!this_process_is_privileged()) {
		auto p = this_process();
		exit(
			execute_with_highest_privileges(p.path(), p.args(), working_dir()));
		return;
	}
	HANDLE token;
	if (OpenProcessToken(
			GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
			AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
		}
		CloseHandle(token);
	}
}

} // namespace _process_privileges

using namespace _process_privileges;

}} // namespace rua::win32

#endif
