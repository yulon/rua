#ifndef _RUA_PROC_WIN32_HPP
#define _RUA_PROC_WIN32_HPP

#ifndef _WIN32
	#error rua::os::proc: not supported this platform!
#endif

#include "../io.hpp"
#include "../any_word.hpp"
#include "../strenc.hpp"
#include "../limits.hpp"
#include "../bin.hpp"
#include "../file/win32.hpp"
#include "../sched.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <cstring>
#include <cassert>

namespace rua { namespace win32 {
	class proc {
		public:
			static proc find(const std::string &name) {
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

			static proc loop_find(const std::string &name) {
				proc p;
				for (;;) {
					p = find(name);
					if (p) {
						break;
					}
					sleep(10);
				}
				return p;
			}

			static proc current() {
				proc tp;
				tp._h = GetCurrentProcess();
				return tp;
			}

			////////////////////////////////////////////////////////////////

			proc() : _h(nullptr), _main_td(nullptr), _need_close(false) {}

			proc(std::nullptr_t) : proc() {}

			proc(HANDLE proc) : _h(proc), _main_td(nullptr), _need_close(false) {}

			static constexpr size_t
				postponed_execution = 0x00000001,
				stdio_pipe = 0x00000010,
				combined_stdout = 0x00000020
			;

			proc(
				const std::string &file,
				const std::vector<std::string> &args,
				const std::string &work_dir,
				uint32_t flags = 0
			) {
				std::wstring file_w(u8_to_w(file));
				std::wstringstream cmd;
				if (args.size()) {
					if (file.find(" ") == std::string::npos) {
						cmd << file_w;
					} else {
						cmd << L"\"" << file_w << L"\"";
					}
					for (auto &arg : args) {
						if (arg.find(" ") == std::string::npos) {
							cmd << L" " << u8_to_w(arg);
						} else {
							cmd << L" \"" << u8_to_w(arg) << L"\"";
						}
					}
				}

				STARTUPINFOW si;
				PROCESS_INFORMATION pi;
				memset(&si, 0, sizeof(si));
				memset(&pi, 0, sizeof(pi));
				si.cb = sizeof(si);
				si.wShowWindow = SW_HIDE;

				win32::file stdo_w, stde_w;

				auto is_capture_stdio = flags & 0x000000F0;
				if (is_capture_stdio) {
					si.dwFlags |= STARTF_USESTDHANDLES;

					SECURITY_ATTRIBUTES pipe_sa;
					pipe_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
					pipe_sa.lpSecurityDescriptor = NULL;
					pipe_sa.bInheritHandle = TRUE;

					if (!CreatePipe(&_stdo_r.native_handle(), &stdo_w.native_handle(), &pipe_sa, 0)) {
						_reset();
						return;
					}
					if (!SetHandleInformation(_stdo_r.native_handle(), HANDLE_FLAG_INHERIT, 0)) {
						_stdo_r.close();
						stdo_w.close();
						_reset();
						return;
					}

					si.hStdOutput = stdo_w.native_handle();

					if (is_capture_stdio < combined_stdout) {
						si.hStdError = stdo_w.native_handle();
					} else {
						if (!CreatePipe(&_stde_r.native_handle(), &stde_w.native_handle(), &pipe_sa, 0)) {
							_stdo_r.close();
							stdo_w.close();
							_reset();
							return;
						}
						if (!SetHandleInformation(_stde_r.native_handle(), HANDLE_FLAG_INHERIT, 0)) {
							_stdo_r.close();
							stdo_w.close();
							_stde_r.close();
							stde_w.close();
							_reset();
							return;
						}
						si.hStdError = stde_w.native_handle();
					}
				}

				if (!CreateProcessW(
					nullptr,
					args.size() ? const_cast<wchar_t *>(cmd.str().c_str()) : nullptr,
					nullptr,
					nullptr,
					true,
					(flags & 0x0000000F) ? CREATE_SUSPENDED : 0,
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

				if (stdo_w) {
					stdo_w.close();
				}
				if (stde_w) {
					stde_w.close();
				}

				_h = pi.hProcess;
				_need_close = true;

				if (flags & 0x0000000F) {
					_main_td = pi.hThread;
				} else {
					CloseHandle(pi.hThread);
					_main_td = nullptr;
				}
			}

			proc(
				const std::string &file,
				const std::vector<std::string> &args,
				uint32_t flags = 0
			) : proc(file, args, "", flags) {}

			proc(
				const std::string &file,
				std::initializer_list<std::string> args,
				uint32_t flags = 0
			) : proc(file, args, "", flags) {}

			proc(
				const std::string &file,
				const std::string &work_dir,
				uint32_t flags = 0
			) : proc(file, {}, work_dir, flags) {}

			proc(
				const std::string &file,
				uint32_t flags = 0
			) : proc(file, {}, "", flags) {}

			proc(DWORD pid) : _main_td(nullptr) {
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

			~proc() {
				reset();
			}

			HANDLE native_handle() const {
				return _h;
			}

			operator HANDLE() const {
				return native_handle();
			}

			operator bool() const {
				return native_handle();
			}

			bool operator==(const proc &target) const {
				return _h == target._h;
			}

			bool operator!=(const proc &target) const {
				return _h != target._h;
			}

			proc(const proc &) = delete;

			proc &operator=(const proc &) = delete;

			proc(proc &&src) :
				_h(src._h), _main_td(src._main_td), _need_close(src._need_close),
				_stdo_r(src._stdo_r), _stde_r(src._stde_r), _stdi_w(src._stdi_w)
			{
				if (src) {
					src._reset();
				}
			}

			proc &operator=(proc &&src) {
				reset();

				if (src) {
					_h = src._h;
					_main_td = src._main_td;
					_need_close = src._need_close;
					_stdo_r = src._stdo_r;
					_stde_r = src._stde_r;
					_stdi_w = src._stdi_w;
					src._reset();
				}

				return *this;
			}

			void start() {
				if (_main_td) {
					ResumeThread(_main_td);
					CloseHandle(_main_td);
					_main_td = nullptr;
				}
			}

			io::reader &stdout_pipe() {
				return _stdo_r;
			}

			io::reader &stderr_pipe() {
				return _stde_r;
			}

			io::writer &stdin_pipe() {
				return _stdi_w;
			}

			int wait_for_exit() {
				if (_h) {
					start();
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

			std::string file_path() {
				WCHAR path[MAX_PATH];
				GetModuleFileNameExW(_h, nullptr, path, MAX_PATH);
				return w_to_u8(path);
			}

			void reset() {
				start();

				if (_h) {
					if (_need_close) {
						CloseHandle(_h);
						_need_close = false;
					}
					if (_stdo_r) {
						_stdo_r.close();
					}
					if (_stde_r) {
						_stde_r.close();
					}
					if (_stdi_w) {
						_stdi_w.close();
					}
					_h = nullptr;
				}
			}

			////////////////////////////////////////////////////////////////

			class mem_view_t : public virtual io::reader_at {
				public:
					mem_view_t() : _proc(nullptr), _ptr(nullptr), _sz(0) {}

					mem_view_t(const proc &proc, any_ptr p, size_t n) : _proc(&proc), _ptr(p), _sz(n) {}

					virtual ~mem_view_t() {}

					const proc &owner() const {
						return *_proc;
					}

					any_ptr ptr() const {
						return _ptr;
					}

					size_t size() const {
						return _sz;
					}

					virtual size_t read_at(ptrdiff_t pos, bin_ref data) {
						SIZE_T sz;
						ReadProcessMemory(_proc->native_handle(), _ptr + pos, data.base(), data.size(), &sz);
						assert(sz <= static_cast<SIZE_T>(nmax<size_t>()));
						return static_cast<size_t>(sz);
					}

					bin_view try_to_local() const {
						if (*_proc != proc::current()) {
							return nullptr;
						}
						return bin_view(_ptr, _sz);
					}

					void reset(proc *proc = nullptr, any_ptr p = nullptr) {
						_proc = proc;
						_ptr = p;
					}

				private:
					const proc *_proc;
					any_ptr _ptr;
					size_t _sz;
			};

			class mem_ref_t : public mem_view_t, public virtual io::read_writer_at {
				public:
					mem_ref_t() : mem_view_t() {}

					mem_ref_t(proc &proc, any_ptr p, size_t n) : mem_view_t(proc, p, n) {}

					virtual ~mem_ref_t() {}

					virtual size_t write_at(ptrdiff_t pos, bin_view data) {
						SIZE_T sz;
						WriteProcessMemory(owner().native_handle(), ptr() + pos, data.base(), data.size(), &sz);
						assert(sz <= static_cast<SIZE_T>(nmax<size_t>()));
						return static_cast<size_t>(sz);
					}

					bin_ref try_to_local() {
						if (owner() != proc::current()) {
							return nullptr;
						}
						return bin_ref(ptr(), size());
					}
			};

			class mem_t : public mem_ref_t {
				public:
					mem_t() : mem_ref_t() {}

					mem_t(proc &proc, size_t n) {
						mem_ref_t::reset(&proc, VirtualAllocEx(proc.native_handle(), nullptr, n, MEM_COMMIT, PAGE_READWRITE));
					}

					virtual ~mem_t() {
						reset();
					}

					virtual void reset() {
						VirtualFreeEx(owner().native_handle(), ptr(), 0, MEM_RELEASE);
						mem_ref_t::reset();
					}
			};

			mem_view_t mem_view(any_ptr p, size_t n) const {
				return mem_view_t(*this, p, n);
			}

			mem_ref_t mem_ref(any_ptr p, size_t n) {
				return mem_ref_t(*this, p, n);
			}

			mem_view_t mem_image() const {
				if (*this == current()) {
					MODULEINFO mi;
					GetModuleInformation(_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
					return mem_view_t(*this, mi.lpBaseOfDll, mi.SizeOfImage);
				}
				assert(false);
				return mem_view_t();
			}

			mem_ref_t mem_image() {
				if (*this == current()) {
					MODULEINFO mi;
					GetModuleInformation(_h, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
					return mem_ref_t(*this, mi.lpBaseOfDll, mi.SizeOfImage);
				}
				assert(false);
				return mem_ref_t();
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

			any_word syscall(any_ptr func, any_word param = nullptr) {
				HANDLE td;
				DWORD tid;
				td = CreateRemoteThread(_h, NULL, 0, func.to<LPTHREAD_START_ROUTINE>(), param, 0, &tid);
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
			file _stdo_r, _stde_r, _stdi_w;

			void _reset() {
				_h = nullptr;
				_main_td = nullptr;
				_need_close = false;
				_stdo_r = nullptr;
				_stde_r = nullptr;
				_stdi_w = nullptr;
			}
	};
}}

#endif
