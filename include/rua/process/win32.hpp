#ifndef _RUA_PROCESS_WIN32_HPP
#define _RUA_PROCESS_WIN32_HPP

#ifndef _WIN32
	#error rua::os::process: not supported this platform!
#endif

#include "../io.hpp"
#include "../any_word.hpp"
#include "../strenc.hpp"
#include "../limits.hpp"
#include "../bin.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <cstring>
#include <cassert>

namespace rua {
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
					Sleep(10);
				}
				return p;
			}

			static process current() {
				process tp;
				tp._ntv_hdl = GetCurrentProcess();
				return tp;
			}

			////////////////////////////////////////////////////////////////

			process() : _ntv_hdl(nullptr), _main_td(nullptr), _need_close(false) {}

			process(std::nullptr_t) : process() {}

			process(HANDLE proc) : _ntv_hdl(proc), _main_td(nullptr), _need_close(false) {}

			process(
				const std::string &file,
				const std::vector<std::string> &args,
				const std::string &pwd,
				bool pause_main_thread = false
			) : _need_close(false) {
				STARTUPINFOW si;
				PROCESS_INFORMATION pi;
				memset(&si, 0, sizeof(si));
				memset(&pi, 0, sizeof(pi));
				si.cb = sizeof(si);

				std::wstring file_w(u8_to_w(file));
				std::wstringstream cmd;
				if (args.size()) {
					cmd << "\"" << file_w << "\"";
					for (auto &arg : args) {
						cmd << " \"" << u8_to_w(arg) << "\"";
					}
				}

				if (!CreateProcessW(
					file_w.c_str(),
					args.size() ? const_cast<wchar_t *>(cmd.str().c_str()) : nullptr,
					nullptr,
					nullptr,
					true,
					pause_main_thread ? CREATE_SUSPENDED : 0,
					nullptr,
					pwd.empty() ? nullptr : u8_to_w(pwd).c_str(),
					&si,
					&pi
				)) {
					_ntv_hdl = nullptr;
					_main_td = nullptr;
					return;
				}

				_ntv_hdl = pi.hProcess;

				if (pause_main_thread) {
					_main_td = pi.hThread;
				} else {
					_main_td = nullptr;
				}
			}

			process(
				const std::string &file,
				const std::vector<std::string> &args,
				bool pause_main_thread = false
			) : process(file, args, "", pause_main_thread) {}

			process(
				const std::string &file,
				std::initializer_list<std::string> args,
				bool pause_main_thread = false
			) : process(file, args, "", pause_main_thread) {}

			process(
				const std::string &file,
				const std::string &pwd,
				bool pause_main_thread = false
			) : process(file, {}, pwd, pause_main_thread) {}

			process(
				const std::string &file,
				bool pause_main_thread = false
			) : process(file, {}, "", pause_main_thread) {}

			process(DWORD pid) : _main_td(nullptr) {
				if (!pid) {
					_ntv_hdl = nullptr;
					_need_close = false;
					return;
				}
				_ntv_hdl = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
				_need_close = true;
			}

			~process() {
				reset();
			}

			HANDLE native_handle() const {
				return _ntv_hdl;
			}

			operator HANDLE() const {
				return native_handle();
			}

			operator bool() const {
				return native_handle();
			}

			bool operator==(const process &target) const {
				return _ntv_hdl == target._ntv_hdl;
			}

			bool operator!=(const process &target) const {
				return _ntv_hdl != target._ntv_hdl;
			}

			process(const process &) = delete;

			process &operator=(const process &) = delete;

			process(process &&src) : _ntv_hdl(src._ntv_hdl), _main_td(src._main_td), _need_close(src._need_close) {
				if (src) {
					src._ntv_hdl = nullptr;
					src._main_td = nullptr;
					src._need_close = false;
				}
			}

			process &operator=(process &&src) {
				reset();

				if (src) {
					_ntv_hdl = src._ntv_hdl;
					_main_td = src._main_td;
					_need_close = src._need_close;
					src._ntv_hdl = nullptr;
					src._main_td = nullptr;
					src._need_close = false;
				}

				return *this;
			}

			void resume_main_thread() {
				if (_main_td) {
					ResumeThread(_main_td);
					_main_td = nullptr;
				}
			}

			void wait_for_exit() {
				if (_ntv_hdl) {
					resume_main_thread();
					WaitForSingleObject(_ntv_hdl, INFINITE);
					reset();
				}
			}

			void exit(int code = 1) {
				if (_ntv_hdl) {
					TerminateProcess(_ntv_hdl, code);
					_main_td = nullptr;
					reset();
				}
			}

			std::string file_path() {
				WCHAR path[MAX_PATH];
				GetModuleFileNameExW(_ntv_hdl, nullptr, path, MAX_PATH);
				return w_to_u8(path);
			}

			void reset() {
				resume_main_thread();

				if (_ntv_hdl) {
					if (_need_close) {
						CloseHandle(_ntv_hdl);
						_need_close = false;
					}
					_ntv_hdl = nullptr;
				}
			}

			////////////////////////////////////////////////////////////////

			class mem_view_t : public virtual io::reader_at {
				public:
					mem_view_t() : _proc(nullptr), _ptr(nullptr), _sz(0) {}

					mem_view_t(const process &proc, any_ptr p, size_t n) : _proc(&proc), _ptr(p), _sz(n) {}

					virtual ~mem_view_t() {}

					const process &owner() const {
						return *_proc;
					}

					any_ptr ptr() const {
						return _ptr;
					}

					size_t size() const {
						return _sz;
					}

					virtual intmax_t read_at(ptrdiff_t pos, bin_ref data) {
						SIZE_T sz;
						ReadProcessMemory(_proc->native_handle(), _ptr + pos, data.base(), data.size(), &sz);
						assert(sz <= static_cast<SIZE_T>(nmax<intmax_t>()));
						return static_cast<intmax_t>(sz);
					}

					bin_view try_to_local() const {
						if (*_proc != process::current()) {
							return nullptr;
						}
						return bin_view(_ptr, _sz);
					}

					void reset(process *proc = nullptr, any_ptr p = nullptr) {
						_proc = proc;
						_ptr = p;
					}

				private:
					const process *_proc;
					any_ptr _ptr;
					size_t _sz;
			};

			class mem_ref_t : public mem_view_t, public virtual io::read_writer_at {
				public:
					mem_ref_t() : mem_view_t() {}

					mem_ref_t(process &proc, any_ptr p, size_t n) : mem_view_t(proc, p, n) {}

					virtual ~mem_ref_t() {}

					virtual intmax_t write_at(ptrdiff_t pos, bin_view data) {
						SIZE_T sz;
						WriteProcessMemory(owner().native_handle(), ptr() + pos, data.base(), data.size(), &sz);
						assert(sz <= static_cast<SIZE_T>(nmax<intmax_t>()));
						return static_cast<intmax_t>(sz);
					}

					bin_ref try_to_local() {
						if (owner() != process::current()) {
							return nullptr;
						}
						return bin_ref(ptr(), size());
					}
			};

			class mem_t : public mem_ref_t {
				public:
					mem_t() : mem_ref_t() {}

					mem_t(process &proc, size_t n) {
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
					GetModuleInformation(_ntv_hdl, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
					return mem_view_t(*this, mi.lpBaseOfDll, mi.SizeOfImage);
				}
				assert(false);
				return mem_view_t();
			}

			mem_ref_t mem_image() {
				if (*this == current()) {
					MODULEINFO mi;
					GetModuleInformation(_ntv_hdl, GetModuleHandleW(nullptr), &mi, sizeof(MODULEINFO));
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
				td = CreateRemoteThread(_ntv_hdl, NULL, 0, func.to<LPTHREAD_START_ROUTINE>(), param, 0, &tid);
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
			HANDLE _ntv_hdl, _main_td;
			bool _need_close;
	};
}

#endif
