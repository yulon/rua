#ifndef _RUA_PROCESS_WIN32_HPP
#define _RUA_PROCESS_WIN32_HPP

#include "../any_word.hpp"
#include "../bytes.hpp"
#include "../io.hpp"
#include "../limits.hpp"
#include "../macros.hpp"
#include "../pipe.hpp"
#include "../sched.hpp"
#include "../stdio.hpp"
#include "../string/encoding/base/win32.hpp"
#include "../string/string_view.hpp"
#include "../thread.hpp"
#include "../type_traits/std_patch.hpp"

#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>

#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace rua { namespace win32 {

using pid_t = DWORD;

class process {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	static process find(string_view name) {
		std::wstring wname(u8_to_w(name));

		auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return nullptr;
		}

		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(PROCESSENTRY32W);
		Process32FirstW(snapshot, &entry);
		do {
			if (wname == entry.szExeFile) {
				CloseHandle(snapshot);
				if (entry.th32ProcessID) {
					return process(entry.th32ProcessID);
				}
			}
		} while (Process32NextW(snapshot, &entry));

		return nullptr;
	}

	static process wait_for_found(string_view name, size_t interval = 100) {
		process p;
		for (;;) {
			p = find(name);
			if (p) {
				break;
			}
			rua::sleep(interval);
		}
		return p;
	}

	////////////////////////////////////////////////////////////////

	constexpr process() : _h(nullptr), _main_td_h(nullptr) {}

	explicit process(
		string_view file,
		const std::vector<std::string> &args = {},
		string_view work_dir = "",
		bool freeze_at_startup = false,
		writer_i stdout_writer = nullptr,
		writer_i stderr_writer = nullptr,
		reader_i stdin_reader = nullptr) {
		std::wstringstream cmd;
		if (file.find(' ') == std::string::npos) {
			cmd << u8_to_w(file);
		} else {
			cmd << L"\"" << u8_to_w(file) << L"\"";
		}
		if (args.size()) {
			for (auto &arg : args) {
				if (arg.find(' ') == std::string::npos) {
					cmd << L" " << u8_to_w(arg);
				} else {
					cmd << L" \"" << u8_to_w(arg) << L"\"";
				}
			}
		}

		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;

		auto stdo_r = std::make_shared<win32::sys_stream>();
		auto stde_r = std::make_shared<win32::sys_stream>();
		auto stdi_w = std::make_shared<win32::sys_stream>();
		win32::sys_stream stdo_w, stde_w, stdi_r;

		bool is_combined_stdout = false;

		if (stdout_writer || stderr_writer || stdin_reader) {
			si.dwFlags = STARTF_USESTDHANDLES;

			auto cph = GetCurrentProcess();

			if (stdout_writer == stderr_writer) {
				is_combined_stdout = true;
			}

			if (!stdout_writer) {
				stdout_writer = rua::get_stdout();
			}
			if (stdout_writer) {
				auto stdout_writer_for_sys =
					stdout_writer.as<win32::sys_stream>();
				if (stdout_writer_for_sys) {
					DuplicateHandle(
						cph,
						stdout_writer_for_sys->native_handle(),
						cph,
						&stdo_w.native_handle(),
						0,
						TRUE,
						DUPLICATE_SAME_ACCESS);
				} else {
					if (!make_pipe(*stdo_r, stdo_w) ||
						!SetHandleInformation(
							stdo_w.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
						_reset();
						return;
					}
				}
			}

			if (is_combined_stdout) {
				DuplicateHandle(
					cph,
					stdo_w.native_handle(),
					cph,
					&stde_w.native_handle(),
					0,
					TRUE,
					DUPLICATE_SAME_ACCESS);
			} else {
				if (!stderr_writer) {
					stderr_writer = rua::get_stderr();
				}
				if (stderr_writer) {
					auto stderr_writer_for_sys =
						stderr_writer.as<win32::sys_stream>();
					if (stderr_writer_for_sys) {
						DuplicateHandle(
							cph,
							stderr_writer_for_sys->native_handle(),
							cph,
							&stde_w.native_handle(),
							0,
							TRUE,
							DUPLICATE_SAME_ACCESS);
					} else {
						if (!make_pipe(*stde_r, stde_w) ||
							!SetHandleInformation(
								stde_w.native_handle(),
								HANDLE_FLAG_INHERIT,
								HANDLE_FLAG_INHERIT)) {
							_reset();
							return;
						}
					}
				}
			}

			if (!stdin_reader) {
				stdin_reader = rua::get_stdin();
			}
			if (stdin_reader) {
				auto stdin_reader_for_sys =
					stdin_reader.as<win32::sys_stream>();
				;
				if (stdin_reader_for_sys) {
					DuplicateHandle(
						cph,
						stdin_reader_for_sys->native_handle(),
						cph,
						&stdi_r.native_handle(),
						0,
						TRUE,
						DUPLICATE_SAME_ACCESS);
				} else {
					if (!make_pipe(stdi_r, *stdi_w) ||
						!SetHandleInformation(
							stdi_r.native_handle(),
							HANDLE_FLAG_INHERIT,
							HANDLE_FLAG_INHERIT)) {
						_reset();
						return;
					}
				}
			}

			si.hStdOutput = stdo_w.native_handle();
			si.hStdError = stde_w.native_handle();
			si.hStdInput = stdi_r.native_handle();
		}

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));

		if (!CreateProcessW(
				nullptr,
				const_cast<wchar_t *>(cmd.str().c_str()),
				nullptr,
				nullptr,
				true,
				freeze_at_startup ? CREATE_SUSPENDED : 0,
				nullptr,
				work_dir.empty() ? nullptr : u8_to_w(work_dir).c_str(),
				&si,
				&pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			_reset();
			return;
		}

		if (*stdo_r) {
			thread([&stdout_writer, stdo_r]() mutable {
				stdout_writer->copy(*stdo_r);
			});
		}
		if (*stde_r) {
			thread([&stderr_writer, stde_r]() mutable {
				stderr_writer->copy(*stde_r);
			});
		}
		if (*stdi_w) {
			thread([stdi_w, &stdin_reader]() mutable {
				stdi_w->copy(stdin_reader);
			});
		}

		_h = pi.hProcess;

		if (freeze_at_startup) {
			_main_td_h = pi.hThread;
		} else {
			CloseHandle(pi.hThread);
			_main_td_h = nullptr;
		}
	}

	explicit process(pid_t id) :
		_h(id ? OpenProcess(PROCESS_ALL_ACCESS, FALSE, id) : nullptr),
		_main_td_h(nullptr) {}

	constexpr process(std::nullptr_t) : process() {}

	template <
		typename T,
		typename =
			enable_if_t<std::is_same<decay_t<T>, native_handle_t>::value>>
	explicit process(T process) : _h(process), _main_td_h(nullptr) {}

	~process() {
		reset();
	}

	process(const process &src) {
		if (!src) {
			_reset();
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
		if (!src._main_td_h) {
			_main_td_h = nullptr;
			return;
		}
		DuplicateHandle(
			GetCurrentProcess(),
			src._main_td_h,
			GetCurrentProcess(),
			&_main_td_h,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
	}

	process(process &&src) : _h(src._h), _main_td_h(src._main_td_h) {
		if (src) {
			src._reset();
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(process)

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

	void unfreeze() {
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
		unfreeze();
		WaitForSingleObject(_h, INFINITE);
		DWORD exit_code;
		GetExitCodeProcess(_h, &exit_code);
		reset();
		return static_cast<int>(exit_code);
	}

	void exit(int code = 1) {
		if (!_h) {
			return;
		}
		TerminateProcess(_h, code);
		_main_td_h = nullptr;
		reset();
	}

	using native_module_handle_t = HMODULE;

	std::string file_path(native_module_handle_t mdu = nullptr) {
		WCHAR path[MAX_PATH + 1];
		auto path_sz = GetModuleFileNameExW(_h, mdu, path, MAX_PATH);
		return w_to_u8(std::wstring(path, path_sz));
	}

	void reset() {
		unfreeze();
		if (!_h) {
			return;
		}
		CloseHandle(_h);
		_h = nullptr;
	}

	////////////////////////////////////////////////////////////////

	class mem_t {
	public:
		mem_t() {}

		mem_t(process &p, size_t n) :
			_owner(&p),
			_ptr(VirtualAllocEx(
				p.native_handle(), nullptr, n, MEM_COMMIT, PAGE_READWRITE)),
			_sz(n) {}

		~mem_t() {
			reset();
		}

		void *ptr() const {
			return _ptr;
		}

		size_t size() const {
			return _sz;
		}

		size_t write_at(ptrdiff_t pos, bytes_view data) {
			SIZE_T sz;
			WriteProcessMemory(
				_owner->native_handle(),
				reinterpret_cast<void *>(
					reinterpret_cast<uintptr_t>(_ptr) + pos),
				data.data(),
				data.size(),
				&sz);
			assert(sz <= static_cast<SIZE_T>(nmax<size_t>()));
			return static_cast<size_t>(sz);
		}

		void reset() {
			if (!_owner) {
				return;
			}
			VirtualFreeEx(_owner->native_handle(), _ptr, _sz, MEM_RELEASE);
			_owner = nullptr;
			_ptr = nullptr;
			_sz = 0;
		}

	private:
		const process *_owner;
		void *_ptr;
		size_t _sz;
	};

	bytes_view mem_image() const {
		assert(id() == GetCurrentProcessId());

		MODULEINFO mi;
		GetModuleInformation(
			_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return bytes_view(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	mem_t mem_alloc(size_t size) {
		return mem_t(*this, size);
	}

	mem_t mem_alloc(string_view str) {
		auto sz = str.length() + 1;
		mem_t data(*this, sz);
		data.write_at(0, bytes_view(str.data(), sz));
		return data;
	}

	mem_t mem_alloc(wstring_view wstr) {
		auto sz = (wstr.length() + 1) * sizeof(wchar_t);
		mem_t data(*this, sz);
		data.write_at(0, bytes_view(wstr.data(), sz));
		return data;
	}

	any_word call(void *func, any_word param = nullptr) {
		HANDLE td;
		DWORD tid;
		td = CreateRemoteThread(
			_h,
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(func),
			param,
			0,
			&tid);
		if (!td) {
			return 0;
		}
		WaitForSingleObject(td, INFINITE);
		DWORD result;
		GetExitCodeThread(td, &result);
		CloseHandle(td);
		return result;
	}

private:
	HANDLE _h, _main_td_h;

	void _reset() {
		_h = nullptr;
		_main_td_h = nullptr;
	}
};

namespace _this_process {

RUA_FORCE_INLINE pid_t this_process_id() {
	return GetCurrentProcessId();
}

RUA_FORCE_INLINE process this_process() {
	return process(this_process_id());
}

} // namespace _this_process

using namespace _this_process;

}} // namespace rua::win32

#endif
