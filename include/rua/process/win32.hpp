#ifndef _RUA_PROC_WIN32_HPP
#define _RUA_PROC_WIN32_HPP

#ifndef _WIN32
	#error rua::process: not supported this platform!
#endif

#include "../io.hpp"
#include "../pipe.hpp"
#include "../stdio.hpp"
#include "../any_word.hpp"
#include "../str.hpp"
#include "../limits.hpp"
#include "../bin.hpp"
#include "../sched.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <functional>
#include <cstring>
#include <memory>
#include <cassert>

namespace rua {
namespace win32 {

class process {
public:
	static process find(const std::string &name) {
		std::wstring wname(u8_to_w(name));
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32W entry;
			entry.dwSize = sizeof(PROCESSENTRY32W);
			Process32FirstW(snapshot, &entry);
			do {
				if (wname == entry.szExeFile) {
					CloseHandle(snapshot);
					if (entry.th32ProcessID) {
						return entry.th32ProcessID;
					}
				}
			} while (Process32NextW(snapshot, &entry));
		}
		return nullptr;
	}

	static process loop_find(const std::string &name) {
		process p;
		for (;;) {
			p = find(name);
			if (p) {
				break;
			}
			sleep(10);
		}
		return p;
	}

	static process current() {
		process tp;
		tp._h = GetCurrentProcess();
		return tp;
	}

	////////////////////////////////////////////////////////////////

	process() : _h(nullptr), _main_td(nullptr), _need_close(false) {}

	process(std::nullptr_t) : process() {}

	process(HANDLE process) : _h(process), _main_td(nullptr), _need_close(false) {}

	process(
		const std::string &file,
		const std::vector<std::string> &args = {},
		const std::string &work_dir = "",
		bool pause_main_thread = false,
		io::writer_i stdout_writer = nullptr,
		io::writer_i stderr_writer = nullptr,
		io::reader_i stdin_reader = nullptr
	) {
		std::wstringstream cmd;
		if (file.find(" ") == std::string::npos) {
			cmd << u8_to_w(file);
		} else {
			cmd << L"\"" << u8_to_w(file) << L"\"";
		}
		if (args.size()) {
			for (auto &arg : args) {
				if (arg.find(" ") == std::string::npos) {
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

		auto stdo_r = std::make_shared<io::win32_stream>();
		auto stde_r = std::make_shared<io::win32_stream>();
		auto stdi_w = std::make_shared<io::win32_stream>();
		io::win32_stream stdo_w, stde_w, stdi_r;

		bool is_combined_stdout = false;

		if (stdout_writer || stderr_writer || stdin_reader) {
			si.dwFlags = STARTF_USESTDHANDLES;

			auto cph = GetCurrentProcess();

			if (stdout_writer == stderr_writer) {
				is_combined_stdout = true;
			}

			if (!stdout_writer) {
				stdout_writer = rua::stdout_writer();
			}
			if (stdout_writer) {
				auto stdout_writer_for_sys = stdout_writer.to<io::win32_stream>();
				if (stdout_writer_for_sys) {
					DuplicateHandle(
						cph,
						stdout_writer_for_sys->native_handle(),
						cph,
						&stdo_w.native_handle(),
						0,
						TRUE,
						DUPLICATE_SAME_ACCESS
					);
				} else {
					if (!make_pipe(*stdo_r, stdo_w) || !SetHandleInformation(stdo_w.native_handle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
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
					DUPLICATE_SAME_ACCESS
				);
			} else {
				if (!stderr_writer) {
					stderr_writer = rua::stderr_writer();
				}
				if (stderr_writer) {
					auto stderr_writer_for_sys = stderr_writer.to<io::win32_stream>();
					if (stderr_writer_for_sys) {
						DuplicateHandle(
							cph,
							stderr_writer_for_sys->native_handle(),
							cph,
							&stde_w.native_handle(),
							0,
							TRUE,
							DUPLICATE_SAME_ACCESS
						);
					} else {
						if (!make_pipe(*stde_r, stde_w) || !SetHandleInformation(stde_w.native_handle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
							_reset();
							return;
						}
					}
				}
			}

			if (!stdin_reader) {
				stdin_reader = rua::stdin_reader();
			}
			if (stdin_reader) {
				auto stdin_reader_for_sys = stdin_reader.to<io::win32_stream>();;
				if (stdin_reader_for_sys) {
					DuplicateHandle(
						cph,
						stdin_reader_for_sys->native_handle(),
						cph,
						&stdi_r.native_handle(),
						0,
						TRUE,
						DUPLICATE_SAME_ACCESS
					);
				} else {
					if (!make_pipe(stdi_r, *stdi_w) || !SetHandleInformation(stdi_r.native_handle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
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
			pause_main_thread ? CREATE_SUSPENDED : 0,
			nullptr,
			work_dir.empty() ? nullptr : u8_to_w(work_dir).c_str(),
			&si,
			&pi
		)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			_reset();
			return;
		}

		if (*stdo_r) {
			std::thread([&stdout_writer, stdo_r]() mutable {
				stdout_writer->copy(*stdo_r);
			}).detach();
		}
		if (*stde_r) {
			std::thread([&stderr_writer, stde_r]() mutable {
				stderr_writer->copy(*stde_r);
			}).detach();
		}
		if (*stdi_w) {
			std::thread([stdi_w, &stdin_reader]() mutable {
				stdi_w->copy(stdin_reader);
			}).detach();
		}

		_h = pi.hProcess;
		_need_close = true;

		if (pause_main_thread) {
			_main_td = pi.hThread;
		} else {
			CloseHandle(pi.hThread);
			_main_td = nullptr;
		}
	}

	process(DWORD pid) : _main_td(nullptr) {
		if (!pid) {
			_h = nullptr;
			_need_close = false;
			return;
		}
		if (pid == GetCurrentProcessId()) {
			_h = GetCurrentProcess();
			return;
		}
		_h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		_need_close = true;
	}

	~process() {
		reset();
	}

	using native_handle_t = HANDLE;

	native_handle_t native_handle() const {
		return _h;
	}

	operator HANDLE() const {
		return native_handle();
	}

	operator bool() const {
		return native_handle();
	}

	bool operator==(const process &target) const {
		return _h == target._h;
	}

	bool operator!=(const process &target) const {
		return _h != target._h;
	}

	process(const process &) = delete;

	process &operator=(const process &) = delete;

	process(process &&src) :
		_h(src._h), _main_td(src._main_td), _need_close(src._need_close)
	{
		if (src) {
			src._reset();
		}
	}

	process &operator=(process &&src) {
		reset();

		if (src) {
			_h = src._h;
			_main_td = src._main_td;
			_need_close = src._need_close;
			src._reset();
		}

		return *this;
	}

	void resume_main_thread() {
		if (_main_td) {
			ResumeThread(_main_td);
			CloseHandle(_main_td);
			_main_td = nullptr;
		}
	}

	int wait_for_exit() {
		assert(_h);
		if (_h) {
			resume_main_thread();
			WaitForSingleObject(_h, INFINITE);
			DWORD exit_code;
			GetExitCodeProcess(_h, &exit_code);
			reset();
			return exit_code;
		}
		return -1;
	}

	void exit(int code = 1) {
		if (_h) {
			TerminateProcess(_h, code);
			_main_td = nullptr;
			reset();
		}
	}

	using native_module_handle_t = HMODULE;

	std::string file_path(native_module_handle_t mdu = nullptr) {
		WCHAR path[MAX_PATH + 1];
		auto path_sz = GetModuleFileNameExW(_h, mdu, path, MAX_PATH);
		return w_to_u8(std::wstring(path, path_sz));
	}

	void reset() {
		resume_main_thread();

		if (_h) {
			if (_need_close) {
				CloseHandle(_h);
				_need_close = false;
			}
			_h = nullptr;
		}
	}

	////////////////////////////////////////////////////////////////

	class mem_t {
		public:
			mem_t() {}

			mem_t(process &process, size_t n) :
				_owner(&process),
				_ptr(VirtualAllocEx(process.native_handle(), nullptr, n, MEM_COMMIT, PAGE_READWRITE)),
				_sz(n)
			{}

			~mem_t() {
				reset();
			}

			any_ptr ptr() const {
				return _ptr;
			}

			size_t size() const {
				return _sz;
			}

			size_t write_at(ptrdiff_t pos, bin_view data) {
				SIZE_T sz;
				WriteProcessMemory(_owner->native_handle(), ptr() + pos, data.base(), data.size(), &sz);
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
			any_ptr _ptr;
			size_t _sz;
	};

	bin_view mem_image() const {
		assert(*this == current());
		MODULEINFO mi;
		GetModuleInformation(_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
		return bin_view(mi.lpBaseOfDll, mi.SizeOfImage);
	}

	mem_t mem_alloc(size_t size) {
		return mem_t(*this, size);
	}

	mem_t mem_alloc(const std::string &str) {
		auto sz = str.length() + 1;
		mem_t data(*this, sz);
		data.write_at(0, bin_view(str.c_str(), sz));
		return data;
	}

	mem_t mem_alloc(const std::wstring &wstr) {
		auto sz = (wstr.length() + 1) * sizeof(wchar_t);
		mem_t data(*this, sz);
		data.write_at(0, bin_view(wstr.c_str(), sz));
		return data;
	}

	any_word call(any_ptr func, any_word param = nullptr) {
		HANDLE td;
		DWORD tid;
		td = CreateRemoteThread(_h, nullptr, 0, func.to<LPTHREAD_START_ROUTINE>(), param, 0, &tid);
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
	HANDLE _h, _main_td;
	bool _need_close;

	void _reset() {
		_h = nullptr;
		_main_td = nullptr;
		_need_close = false;
	}
};

}
}

#endif
