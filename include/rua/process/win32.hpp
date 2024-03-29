#ifndef _rua_process_win32_hpp
#define _rua_process_win32_hpp

#include "base.hpp"

#include "../binary/bytes.hpp"
#include "../dylib/win32.hpp"
#include "../dype/any_ptr.hpp"
#include "../file/win32.hpp"
#include "../hard/win32.hpp"
#include "../io.hpp"
#include "../memory.hpp"
#include "../stdio/win32.hpp"
#include "../string/case.hpp"
#include "../string/char_set.hpp"
#include "../string/codec/base/win32.hpp"
#include "../string/join.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"
#include "../conc/await.hpp"
#include "../conc/future.hpp"
#include "../conc/then.hpp"
#include "../conc/wait.hpp"
#include "../sys/info/win32.hpp"
#include "../sys/stream/win32.hpp"
#include "../sys/wait/win32.hpp"
#include "../thread/sleep/win32.hpp"
#include "../util.hpp"

#include <psapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <winternl.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
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

class process : private enable_await_operators {
public:
	using native_handle_t = HANDLE;

	template <
		typename Pid,
		typename = enable_if_t<std::is_integral<Pid>::value>>
	explicit process(Pid id) {
		if (!id) {
			$h = nullptr;
			return;
		}

		$h = OpenProcess($all_access(), false, static_cast<pid_t>(id));
		if ($h) {
			return;
		}

		static auto has_dbg_priv = ([]() -> bool {
			HANDLE token;
			if (!OpenProcessToken(
					GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
				return false;
			}
			TOKEN_PRIVILEGES tp;
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			auto r = LookupPrivilegeValueW(
				nullptr, L"SeDebugPrivilege", &tp.Privileges[0].Luid);
			if (r) {
				r = AdjustTokenPrivileges(
					token, false, &tp, sizeof(tp), nullptr, nullptr);
				if (r) {
					r = GetLastError() != ERROR_NOT_ALL_ASSIGNED;
				}
			}
			CloseHandle(token);
			return r;
		})();
		if (has_dbg_priv) {
			$h = OpenProcess($all_access(), false, static_cast<pid_t>(id));
			if ($h) {
				return;
			}
		}

		$h = OpenProcess($read_access(), false, static_cast<pid_t>(id));
	}

	constexpr process(std::nullptr_t = nullptr) : $h(nullptr) {}

	explicit process(native_handle_t h) : $h(h) {}

	~process() {
		reset();
	}

	process(process &&src) :
		$h(src.$h),
		$prev_get_cpu_usage_time(src.$prev_get_cpu_usage_time),
		$prev_used_cpu_time(src.$prev_used_cpu_time) {
		if (src) {
			src.$h = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(process)

	pid_t id() const {
		return GetProcessId($h);
	}

	bool operator==(const process &target) const {
		return id() == target.id();
	}

	bool operator!=(const process &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		return $h;
	}

	operator bool() const {
		return $h;
	}

	future<any_word> RUA_OPERATOR_AWAIT() const {
		auto h = $h;
		return sys_wait(h) >> [h]() -> any_word {
			DWORD ec;
			GetExitCodeProcess(h, &ec);
			return ec;
		};
	}

	void kill() {
		if (!$h) {
			return;
		}
		TerminateProcess($h, 0);
		$reset();
	}

	bool is_suspended() const {
		auto nt_query_system_information = $ntdll().nt_query_system_information;
		if (!nt_query_system_information) {
			return false;
		}

		DWORD si_buf_sz = 0;

		nt_query_system_information(5, 0, 0, &si_buf_sz);
		if (!si_buf_sz) {
			return false;
		}

		si_buf_sz += 0x10000;
		bytes si_buf(si_buf_sz);

		if (nt_query_system_information(
				5, si_buf.data(), si_buf_sz, &si_buf_sz) < 0) {
			return false;
		}

		auto si_data = si_buf(0, si_buf_sz);

		auto pid = id();
		for (;;) {
			struct system_process_information {
				struct unicode_string {
					USHORT Length;
					USHORT MaximumLength;
					PWSTR Buffer;
				};
				ULONG NextEntryOffset;
				ULONG NumberOfThreads;
				BYTE Reserved1[48];
				unicode_string ImageName;
				LONG BasePriority;
				HANDLE UniqueProcessId;
				PVOID Reserved2;
				ULONG HandleCount;
				ULONG SessionId;
				PVOID Reserved3;
				SIZE_T PeakVirtualSize;
				SIZE_T VirtualSize;
				ULONG Reserved4;
				SIZE_T PeakWorkingSetSize;
				SIZE_T WorkingSetSize;
				PVOID Reserved5;
				SIZE_T QuotaPagedPoolUsage;
				PVOID Reserved6;
				SIZE_T QuotaNonPagedPoolUsage;
				SIZE_T PagefileUsage;
				SIZE_T PeakPagefileUsage;
				SIZE_T PrivatePageCount;
				LARGE_INTEGER Reserved7[6];
			};
			auto &spi = si_data.as<system_process_information>();
			if (static_cast<pid_t>(
					reinterpret_cast<uintptr_t>(spi.UniqueProcessId)) != pid) {
				if (!spi.NextEntryOffset) {
					return false;
				}
				si_data = si_data(spi.NextEntryOffset);
				continue;
			}
			si_data = si_data(sizeof(system_process_information));
			for (size_t i = 0; i < spi.NumberOfThreads; ++i) {
				struct system_thread_information {
					struct client_id {
						HANDLE UniqueProcess;
						HANDLE UniqueThread;
					};
					LARGE_INTEGER Reserved1[3];
					ULONG Reserved2;
					PVOID StartAddress;
					client_id ClientId;
					LONG Priority;
					LONG BasePriority;
					ULONG Reserved3;
					ULONG ThreadState;
					ULONG WaitReason;
				};
				auto &sti = si_data.aligned_as<system_thread_information>(i);
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
		auto nt_suspend_process = $ntdll().nt_suspend_process;
		if (!nt_suspend_process) {
			return false;
		}
		return nt_suspend_process($h) >= 0;
	}

	bool resume() {
		auto nt_resume_process = $ntdll().nt_resume_process;
		if (!nt_resume_process) {
			return false;
		}
		return nt_resume_process($h) >= 0;
	}

	using native_module_handle_t = HMODULE;

	file_path path(native_module_handle_t mdu = nullptr) const {
		auto get_module_file_name_ex_w = $psapi().get_module_file_name_ex_w;
		if (!get_module_file_name_ex_w) {
			return "";
		}
		WCHAR pth[MAX_PATH];
		auto pth_sz = get_module_file_name_ex_w($h, mdu, pth, MAX_PATH);
		if (!pth_sz) {
			return "";
		}
		return w2u({pth, pth_sz});
	}

	std::string name(native_module_handle_t mdu = nullptr) const {
		auto get_module_base_name_w = $psapi().get_module_base_name_w;
		if (!get_module_base_name_w) {
			return path().back();
		}
		WCHAR n[MAX_PATH];
		auto n_sz = get_module_base_name_w($h, mdu, n, MAX_PATH);
		if (!n_sz) {
			return path().back();
		}
		return w2u({n, n_sz});
	}

	std::vector<std::string> args() const {
		std::vector<std::string> params;

		int argc;
		LPWSTR *argv;

		if (id() == this_pid()) {
			argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		} else {
			auto nt_query_information_process =
				$ntdll().nt_query_information_process;
			if (!nt_query_information_process) {
				return params;
			}

			$process_basic_information pbi;
			if (nt_query_information_process(
					$h, 0, &pbi, sizeof(pbi), nullptr) < 0) {
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
				params.emplace_back(w2u(argv[i]));
			}
		}

		LocalFree(argv);

		return params;
	}

	pid_t parent_id() const {
		auto nt_query_information_process =
			$ntdll().nt_query_information_process;
		if (!nt_query_information_process) {
			return 0;
		}
		$process_basic_information pbi;
		if (nt_query_information_process($h, 0, &pbi, sizeof(pbi), nullptr) <
			0) {
			return 0;
		}
		return static_cast<pid_t>(pbi.InheritedFromUniqueProcessId);
	}

	process parent() const {
		return process(parent_id());
	}

	float
	cpu_usage(duration &prev_get_time, duration &prev_used_cpu_time) const {
		FILETIME CreateTime, ExitTime, KernelTime, UserTime;
		if (!GetProcessTimes(
				$h, &CreateTime, &ExitTime, &KernelTime, &UserTime)) {
			return 0.f;
		}

		duration used_cpu_time(
			((static_cast<int64_t>(KernelTime.dwHighDateTime) << 32 |
			  static_cast<int64_t>(KernelTime.dwLowDateTime)) +
			 (static_cast<int64_t>(UserTime.dwHighDateTime) << 32 |
			  static_cast<int64_t>(UserTime.dwLowDateTime))) /
			10000);

		auto now = rua::tick();

		auto r =
			prev_get_time && prev_used_cpu_time
				? static_cast<float>(
					  static_cast<double>(
						  (used_cpu_time - prev_used_cpu_time).milliseconds()) /
					  static_cast<double>(
						  (now - prev_get_time).milliseconds()) /
					  static_cast<double>(num_cpus()))
				: 0.f;

		prev_used_cpu_time = used_cpu_time;
		prev_get_time = now;

		return r;
	}

	float cpu_usage() {
		return cpu_usage($prev_get_cpu_usage_time, $prev_used_cpu_time);
	}

	size_t memory_usage() const {
		auto get_process_memoryinfo = $psapi().get_process_memoryinfo;
		if (!get_process_memoryinfo) {
			return 0;
		}
		PROCESS_MEMORY_COUNTERS pmc;
		return get_process_memoryinfo($h, &pmc, sizeof(pmc))
				   ? pmc.WorkingSetSize
				   : 0;
	}

	ptrdiff_t memory_read(any_ptr ptr, bytes_ref data) const {
		SIZE_T sz;
		if (!ReadProcessMemory($h, ptr, data.data(), data.size(), &sz)) {
			return -1;
		}
		return static_cast<ptrdiff_t>(sz);
	}

	ptrdiff_t memory_write(any_ptr ptr, bytes_view data) {
		SIZE_T sz;
		if (!WriteProcessMemory($h, ptr, data.data(), data.size(), &sz)) {
			return -1;
		}
		return static_cast<ptrdiff_t>(sz);
	}

	class memory_block {
	public:
		constexpr memory_block() :
			$h(nullptr), $p(nullptr), $sz(0), $need_free(false) {}

		~memory_block() {
			reset();
		}

		operator bool() const {
			return $p;
		}

		any_ptr data() const {
			return $p;
		}

		size_t size() const {
			return $sz;
		}

		ptrdiff_t read_at(ptrdiff_t pos, bytes_ref data) {
			SIZE_T sz;
			if (!ReadProcessMemory(
					$h, $p + pos, data.data(), data.size(), &sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		ptrdiff_t write_at(ptrdiff_t pos, bytes_view data) {
			SIZE_T sz;
			if (!WriteProcessMemory(
					$h, $p + pos, data.data(), data.size(), &sz)) {
				return -1;
			}
			return static_cast<ptrdiff_t>(sz);
		}

		void detach() {
			$need_free = false;
		}

		void reset() {
			if ($need_free) {
				VirtualFreeEx($h, $p, $sz, MEM_RELEASE);
				$need_free = false;
			}
			$h = nullptr;
			$p = nullptr;
			$sz = 0;
		}

	private:
		native_handle_t $h;
		any_ptr $p;
		size_t $sz;
		bool $need_free;

		memory_block(
			native_handle_t h, any_ptr data, size_t n, bool need_free) :
			$h(h), $p(data), $sz(n), $need_free(need_free) {}

		friend process;
	};

	memory_block memory_ref(any_ptr data, size_t size) const {
		return memory_block($h, data, size, false);
	}

	memory_block memory_image(native_module_handle_t mdu = nullptr) const {
		auto get_module_information = $psapi().get_module_information;
		if (!get_module_information) {
			return {};
		}
		MODULEINFO mi;
		get_module_information(
			$h, mdu ? mdu : GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return memory_ref(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	memory_block memory_alloc(size_t size, bool is_executable = true) {
		return memory_block(
			$h,
			VirtualAllocEx(
				$h,
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

	thread make_thread(any_ptr func, any_ptr param = nullptr) {
		if (sys_version() >= 6) {
			auto nt_create_thread_ex = $ntdll().nt_create_thread_ex;
			if (nt_create_thread_ex) {
				HANDLE td;
				if (SUCCEEDED(nt_create_thread_ex(
						&td,
						0x1FFFFF,
						nullptr,
						$h,
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
			CreateRemoteThread($h, nullptr, 0, func, param, 0, nullptr));
	}

	inline thread load_dylib(std::string name);

	void reset() {
		if (!$h) {
			return;
		}
		$reset();
	}

	void detach() {
		if (!$h) {
			return;
		}
		$h = nullptr;
	}

private:
	HANDLE $h;
	duration $prev_get_cpu_usage_time, $prev_used_cpu_time;

	void $reset() {
		assert($h);

		CloseHandle($h);
		$h = nullptr;
	}

	static DWORD $all_access() {
		if (sys_version() >= 6) {
			return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF;
		}
		return STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF;
	}

	static constexpr DWORD $read_access() {
		return PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE;
	}

	struct $process_basic_information {
		NTSTATUS ExitStatus;
		PPEB PebBaseAddress;
		ULONG_PTR AffinityMask;
		LONG BasePriority;
		ULONG_PTR UniqueProcessId;
		ULONG_PTR InheritedFromUniqueProcessId;
	};

	struct $process_extended_basic_information {
		SIZE_T Size;
		$process_basic_information BasicInfo;
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

	struct $ntdll_t {
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
		HRESULT(WINAPI *nt_create_thread_ex)
		(PHANDLE ThreadHandle,
		 ACCESS_MASK DesiredAccess,
		 std::nullptr_t ObjectAttributes,
		 HANDLE ProcessHandle,
		 PVOID StartRoutine,
		 PVOID StartContext,
		 ULONG Flags,
		 ULONG_PTR StackZeroBits,
		 SIZE_T StackCommit,
		 SIZE_T StackReserve,
		 PVOID AttributeList);
	};

	static const $ntdll_t &$ntdll() {
		static auto ntdll = dylib::from_loaded_or_load("ntdll.dll");
		static const $ntdll_t inst{
			ntdll["NtSuspendProcess"],
			ntdll["NtResumeProcess"],
			ntdll["NtQuerySystemInformation"],
			ntdll["NtQueryInformationProcess"],
			ntdll["NtCreateThreadEx"]};
		return inst;
	}

	struct $psapi_t {
		decltype(&GetModuleFileNameExW) get_module_file_name_ex_w;
		decltype(&GetModuleBaseNameW) get_module_base_name_w;
		decltype(&GetModuleInformation) get_module_information;
		decltype(&GetProcessMemoryInfo) get_process_memoryinfo;
	};

	static any_ptr
	$load_psapi(dylib &kernel32, dylib &psapi, string_view name) {
		auto fp = kernel32[join("K32", name)];
		if (fp) {
			return fp;
		}
		if (!psapi) {
			psapi = dylib("psapi.dll");
		}
		return psapi[name];
	}

	static const $psapi_t &$psapi() {
		static dylib kernel32 = dylib::from_loaded_or_load("kernel32.dll"),
					 psapi;
		static const $psapi_t inst{
			$load_psapi(kernel32, psapi, "GetModuleFileNameExW"),
			$load_psapi(kernel32, psapi, "GetModuleBaseNameW"),
			$load_psapi(kernel32, psapi, "GetModuleInformation"),
			$load_psapi(kernel32, psapi, "GetProcessMemoryInfo")};
		return inst;
	}
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
		ctx->rtl_init_unicode_string(&us, names);
		ctx->ldr_load_dll(nullptr, 0, &us, &h);
		names = ++it;
	}

	if (ctx->rtl_user_thread_start) {
#if defined(_MSC_VER) && RUA_X86 == 32
		auto rtl_user_thread_start = ctx->rtl_user_thread_start;
		auto td_start = ctx->td_start;
		auto td_param = ctx->td_param;
		__asm {
				mov eax, td_start
				mov edx, td_param
				call rtl_user_thread_start
		}
#else
		ctx->rtl_user_thread_start(ctx->td_start, ctx->td_param);
#endif
	}

	return h;
}
#endif

// clang-format off
RUA_CVAR const uchar _proc_dll_loader_code[] {
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

struct _proc_load_dll_ctx {
	struct unicode_string;

	HRESULT(WINAPI *rtl_init_unicode_string)(unicode_string *, PCWSTR);
	HRESULT(WINAPI *ldr_load_dll)
	(PWCHAR, ULONG, unicode_string *, HMODULE *);
	const wchar_t *names;

	void(RUA_REGPARM(3) * rtl_user_thread_start)(PTHREAD_START_ROUTINE, PVOID);
	PTHREAD_START_ROUTINE td_start;
	PVOID td_param;
};

struct _proc_load_dll_data {
	process::memory_block loader, names, ctx;

	operator bool() const {
		return loader && names && ctx;
	}
};

template <RUA_STRING_RANGE(StrList)>
_proc_load_dll_data _make_proc_load_dll_data(
	process &proc, const StrList &names, _proc_load_dll_ctx &ctx) {
	_proc_load_dll_data data;

	data.loader = proc.memory_alloc(_proc_dll_loader_code, true);
	if (!data.loader) {
		return data;
	}

	bytes names_buf;
	for (auto &name : names) {
		names_buf += as_bytes(u2w(name));
		names_buf += as_bytes(L'\0');
	}
	names_buf += as_bytes(L'\0');

	data.names = proc.memory_alloc(names_buf);
	if (!data.names) {
		return data;
	}

	ctx.names = data.names.data();

	static auto ntdll = dylib::from_loaded_or_load("ntdll.dll");
	static auto rtl_init_unicode_string = ntdll["RtlInitUnicodeString"];
	static auto ldr_load_dll = ntdll["LdrLoadDll"];

	ctx.rtl_init_unicode_string = rtl_init_unicode_string;
	ctx.ldr_load_dll = ldr_load_dll;

	data.ctx = proc.memory_alloc(as_bytes(ctx));
	if (!data.ctx) {
		return data;
	}

	return data;
}

inline thread process::load_dylib(std::string name) {
	_proc_load_dll_ctx ctx;
	ctx.rtl_user_thread_start = nullptr;

	auto data = _make_proc_load_dll_data(*this, {std::move(name)}, ctx);
	if (!data) {
		return nullptr;
	}

	return make_thread(data.loader.data(), data.ctx.data());
}

namespace _this_process {

inline process this_process() {
	return process(GetCurrentProcess());
}

inline file_path working_dir() {
	return w2u(_get_dir_w(GetCurrentDirectoryW));
}

inline bool work_at(const file_path &path) {
#ifndef NDEBUG
	auto r =
#else
	return
#endif
		SetCurrentDirectoryW(u2w(path.abs().str()).c_str());
#ifndef NDEBUG
	assert(r);
	return r;
#endif
}

inline std::string get_env(string_view name) {
	return w2u(_get_dir_w(GetEnvironmentVariableW, u2w(name)));
}

#if defined(RUA_X86) && RUA_X86 == 32

inline bool is_x86_on_x64() {
	static auto r = ([]() -> bool {
		BOOL(WINAPI * is_wow64_process)
		(HANDLE hProcess, PBOOL Wow64Process) =
			dylib::from_loaded_or_load("kernel32.dll")["IsWow64Process"];
		if (!is_wow64_process) {
			return false;
		}
		BOOL is_wow64;
		return is_wow64_process(GetCurrentProcess(), &is_wow64) && is_wow64;
	})();
	return r;
}

#else

inline constexpr bool is_x86_on_x64() {
	return false;
}

#endif

inline bool has_full_permissions() {
	SID_IDENTIFIER_AUTHORITY sna = SECURITY_NT_AUTHORITY;
	PSID admin_group = nullptr;
	if (!AllocateAndInitializeSid(
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
			&admin_group) ||
		!admin_group) {
		return false;
	}
	BOOL is_member = FALSE;
	auto r = CheckTokenMembership(nullptr, admin_group, &is_member);
	if (r) {
		r = is_member;
	}
	FreeSid(admin_group);
	return r;
}

} // namespace _this_process

using namespace _this_process;

using _process_make_info =
	_baisc_process_make_info<process, file_path, sys_stream>;

namespace _make_process {

class process_maker
	: public process_maker_base<process_maker, _process_make_info> {
public:
	process_maker() = default;

	process_maker(process_maker &&pm) :
		process_maker_base<process_maker, _process_make_info>(
			std::move(static_cast<process_maker_base &&>(pm))),
		$el_perms(false) {}

	RUA_OVERLOAD_ASSIGNMENT(process_maker)

	~process_maker() {
		if (!$info.file) {
			return;
		}
		start();
	}

	process start() {
		if (!$info.file) {
			return nullptr;
		}

		if ($el_perms && sys_version() >= 6 && !has_full_permissions()) {
			if ($info.stdout_w) {
				$info.stdout_w.reset();
			}
			if ($info.stderr_w) {
				$info.stderr_w.reset();
			}
			if ($info.stdin_r) {
				$info.stdin_r.reset();
			}

			SHELLEXECUTEINFOW sei;
			memset(&sei, 0, sizeof(sei));
			sei.cbSize = sizeof(sei);
			sei.lpVerb = L"runas";
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.nShow = $info.hide ? SW_HIDE : SW_SHOW;

			auto path_w = u2w($info.file.str());
			sei.lpFile = path_w.c_str();

			$info.file = "";

			std::wstringstream args_ss_w;
			for (auto &arg : $info.args) {
				if (arg.find(' ') == string_view::npos) {
					args_ss_w << L" " << u2w(arg);
				} else {
					args_ss_w << L" \"" << u2w(arg) << L"\"";
				}
			}
			auto args_w = args_ss_w.str();
			if (args_w.length()) {
				sei.lpParameters = args_w.c_str();
			}

			auto wd_w = u2w($info.work_dir.str());
			sei.lpDirectory = wd_w.c_str();

			if (!ShellExecuteExW(&sei)) {
				return nullptr;
			}
			return process(sei.hProcess);
		}

		std::wstringstream cmd_ss_w;
		if ($info.file.str().find(' ') == std::string::npos) {
			cmd_ss_w << u2w($info.file.str());
		} else {
			cmd_ss_w << L"\"" << u2w($info.file.str()) << L"\"";
		}
		for (auto &arg : $info.args) {
			if (arg.find(' ') == string_view::npos) {
				cmd_ss_w << L" " << u2w(arg);
			} else {
				cmd_ss_w << L" \"" << u2w(arg) << L"\"";
			}
		}

		$info.file = "";

		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);

		if ($info.stdout_w || $info.stderr_w || $info.stdin_r) {
			si.dwFlags |= STARTF_USESTDHANDLES;

			if ($info.stdout_w) {
				if (*$info.stdout_w && !SetHandleInformation(
										   $info.stdout_w->native_handle(),
										   HANDLE_FLAG_INHERIT,
										   HANDLE_FLAG_INHERIT)) {
					$info.stdout_w->close();
				}
				si.hStdOutput = $info.stdout_w->native_handle();
			} else {
				si.hStdOutput = out().native_handle();
			}

			if ($info.stderr_w) {
				if (*$info.stderr_w && !SetHandleInformation(
										   $info.stderr_w->native_handle(),
										   HANDLE_FLAG_INHERIT,
										   HANDLE_FLAG_INHERIT)) {
					$info.stderr_w->close();
				}
				si.hStdError = $info.stderr_w->native_handle();
			} else if ($info.stdout_w) {
				si.hStdError = si.hStdOutput;
			} else {
				si.hStdError = err().native_handle();
			}

			if ($info.stdin_r) {
				if (*$info.stdin_r && !SetHandleInformation(
										  $info.stdin_r->native_handle(),
										  HANDLE_FLAG_INHERIT,
										  HANDLE_FLAG_INHERIT)) {
					$info.stdin_r->close();
				}
				si.hStdInput = $info.stdin_r->native_handle();
			} else {
				si.hStdInput = in().native_handle();
			}
		}

		if ($info.hide) {
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
		}

		auto is_suspended = $info.on_start || $info.dylibs.size();

		DWORD creation_flags = is_suspended ? CREATE_SUSPENDED : 0;

		std::wstring envs;
		if ($info.envs.size()) {
			creation_flags |= CREATE_UNICODE_ENVIRONMENT;

			auto begin = GetEnvironmentStringsW();
			LPWCH eq_begin = nullptr;
			for (auto p = begin;; ++p) {
				if (*p) {
					if (*p == L'=') {
						eq_begin = p;
					}
					continue;
				}
				if (begin == p) {
					break;
				}
				if (eq_begin) {
					if (eq_begin < p - 1) {
						if ($info.envs.find(
								w2u(wstring_view(begin, eq_begin - begin))) ==
							$info.envs.end()) {
							envs += wstring_view(begin, p - begin + 1);
						}
					}
					eq_begin = nullptr;
				}
				begin = p + 1;
			}

			for (auto &env : $info.envs) {
				envs += u2w(join(
					env.first, "=", env.second, rua::string_view("\0", 1)));
			}
		}

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));

		auto ok = CreateProcessW(
			nullptr,
			const_cast<wchar_t *>(cmd_ss_w.str().c_str()),
			nullptr,
			nullptr,
			true,
			creation_flags,
			envs.size() ? &envs[0] : nullptr,
			$info.work_dir ? rua::u2w($info.work_dir.str()).c_str() : nullptr,
			&si,
			&pi);

		if ($info.stdout_w) {
			$info.stdout_w.reset();
		}
		if ($info.stderr_w) {
			$info.stderr_w.reset();
		}
		if ($info.stdin_r) {
			$info.stdin_r.reset();
		}

		if (!ok) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return nullptr;
		}

		process proc(pi.hProcess);

		if (!is_suspended) {
			CloseHandle(pi.hThread);
			return proc;
		}

		$start_with_load_dlls(proc, pi.hThread);

		if ($info.on_start) {
			$info.on_start(proc);
		}

		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
		return proc;
	}

	process_maker &elevate_permissions() {
		$el_perms = true;
		return *this;
	}

private:
	bool $el_perms;

	process_maker(_process_make_info info) :
		process_maker_base(std::move(info)), $el_perms(false) {}

	bool $start_with_load_dlls(process &proc, HANDLE main_td_h) {
		if (!$info.dylibs.size()) {
			return false;
		}

		CONTEXT main_td_ctx;
		main_td_ctx.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(main_td_h, &main_td_ctx)) {
			return false;
		}

		_proc_load_dll_ctx ctx;

#if RUA_X86 == 64
		ctx.rtl_user_thread_start = any_ptr(main_td_ctx.Rip);
		ctx.td_start = any_ptr(main_td_ctx.Rcx);
		ctx.td_param = any_ptr(main_td_ctx.Rdx);
#elif RUA_X86 == 32
		ctx.rtl_user_thread_start = any_ptr(main_td_ctx.Eip);
		ctx.td_start = any_ptr(main_td_ctx.Eax);
		ctx.td_param = any_ptr(main_td_ctx.Edx);
#endif

		auto data = _make_proc_load_dll_data(proc, $info.dylibs, ctx);
		if (!data) {
			return false;
		}

#if RUA_X86 == 64
		main_td_ctx.Rip = data.loader.data().uintptr();
		main_td_ctx.Rcx = data.ctx.data().uintptr();
#elif RUA_X86 == 32
		main_td_ctx.Eip = data.loader.data().uintptr();
		main_td_ctx.Esp -= 2 * sizeof(uintptr_t);
		auto stk =
			proc.memory_ref(any_ptr(main_td_ctx.Esp), 2 * sizeof(uintptr_t));
		if (stk.write_at(
				sizeof(uintptr_t), as_bytes(data.ctx.data().uintptr())) <= 0) {
			return false;
		}
#endif

		if (!SetThreadContext(main_td_h, &main_td_ctx)) {
			return false;
		}

		data.loader.detach();
		data.ctx.detach();
		data.names.detach();

		return true;
	}

	friend process_maker make_process(file_path);
};

inline process_maker make_process(file_path file) {
	return process_maker(_process_make_info(std::move(file)));
}

} // namespace _make_process

using namespace _make_process;

namespace _this_process {

inline void elevate_permissions() {
	if (sys_version() < 6 || has_full_permissions()) {
		return;
	}
	auto p = this_process();
	make_process(p.path())
		.args(p.args())
		.work_at(working_dir())
		.elevate_permissions()
		.start();
	exit(0);
}

} // namespace _this_process

namespace _find_process {

class process_finder : private wandering_iterator {
public:
	process_finder() : $snap(INVALID_HANDLE_VALUE) {}

	~process_finder() {
		if (*this) {
			$reset();
		}
	}

	process_finder(process_finder &&src) : $snap(src.$snap), $ety(src.$ety) {
		if (src) {
			src.$snap = INVALID_HANDLE_VALUE;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(process_finder)

	operator bool() const {
		return $snap != INVALID_HANDLE_VALUE;
	}

	process operator*() const {
		return *this ? process($ety.th32ProcessID) : process();
	}

	process_finder &operator++() {
		assert(*this);

		while (Process32NextW($snap, &$ety)) {
			if (!$ety.th32ProcessID) {
				continue;
			}
			if ((!$id && !$name.size()) || $ety.th32ProcessID == $id ||
				rua::to_lowers($ety.szExeFile) == $name) {
				return *this;
			}
		}
		$reset();
		return *this;
	}

	using native_handle_t = HANDLE;

	native_handle_t native_handle() const {
		return $snap;
	}

	using native_info_t = PROCESSENTRY32W;

	const native_info_t &native_info() const {
		return $ety;
	}

	pid_t id() const {
		return $ety.th32ProcessID;
	}

	pid_t parent_id() const {
		return $ety.th32ParentProcessID;
	}

	std::string name() const {
		return w2u($ety.szExeFile);
	}

private:
	pid_t $id;
	std::wstring $name;
	HANDLE $snap;
	PROCESSENTRY32W $ety;

	process_finder(pid_t id, string_view name) : $id(id) {
		if (!id && name.length()) {
			$name = u2w(rua::to_lowers(name));
		}

		$snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if ($snap == INVALID_HANDLE_VALUE) {
			$ety.th32ProcessID = 0;
			$ety.th32ParentProcessID = 0;
			$ety.szExeFile[0] = 0;
			return;
		}

		$ety.dwSize = sizeof(PROCESSENTRY32W);

		if (!Process32FirstW($snap, &$ety)) {
			$reset();
			return;
		}
		do {
			if (!$ety.th32ProcessID) {
				continue;
			}
			if ((!$id && !$name.size()) || $ety.th32ProcessID == $id ||
				rua::to_lowers($ety.szExeFile) == $name) {
				return;
			}
		} while (Process32NextW($snap, &$ety));
		$reset();
	}

	void $reset() {
		CloseHandle($snap);
		$snap = INVALID_HANDLE_VALUE;
		$ety.th32ProcessID = 0;
		$ety.th32ParentProcessID = 0;
		$ety.szExeFile[0] = 0;
	}

	friend process_finder find_process(pid_t);
	friend process_finder find_process(string_view);
};

inline process_finder find_process(pid_t id) {
	return process_finder(id, "");
}

inline process_finder found_process(pid_t id, duration interval = 100) {
	process_finder pf;
	for (;;) {
		pf = find_process(id);
		if (pf) {
			break;
		}
		sleep(interval);
	}
	return pf;
}

inline process_finder find_process(string_view name) {
	return process_finder(0, name);
}

inline process_finder found_process(string_view name, duration interval = 100) {
	process_finder pf;
	for (;;) {
		pf = find_process(name);
		if (pf) {
			break;
		}
		sleep(interval);
	}
	return pf;
}

inline std::string process_name(pid_t id) {
	process proc(id);
	if (proc) {
		return proc.name();
	}
	return find_process(id).name();
}

inline file_path process_path(pid_t id) {
	process proc(id);
	if (proc) {
		return proc.path();
	}
	return find_process(id).name();
}

} // namespace _find_process

using namespace _find_process;

}} // namespace rua::win32

#endif
