#ifndef _RUA_PROCESS_HPP
#define _RUA_PROCESS_HPP

#include "io/data.hpp"
#include "gnc/any_word.hpp"

#ifdef _WIN32
	#include "strenc.hpp"

	#include <windows.h>
	#include <tlhelp32.h>
	#include <psapi.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <cstring>
#include <cassert>

namespace rua {
	#ifdef _WIN32
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

				static process from_this() {
					process tp;
					tp._ntv_hdl = GetCurrentProcess();
					return tp;
				}

				////////////////////////////////////////////////////////////////

				process() : _ntv_hdl(nullptr), _main_td(nullptr), _need_close(false), _mem_mgr(nullptr) {}

				process(std::nullptr_t) : process() {}

				process(HANDLE proc) : _ntv_hdl(proc), _main_td(nullptr), _need_close(false), _mem_mgr(nullptr) {
					if (!proc) {
						return;
					}
					if (*this != process::from_this()) {
						_mem_mgr = mem_mgr_t(*this);
					}
				}

				process(
					const std::string &file,
					const std::vector<std::string> &args,
					const std::string &pwd,
					bool pause_main_thread = false
				) : _need_close(false), _mem_mgr(nullptr) {
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

					_mem_mgr = mem_mgr_t(*this);
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

				process(DWORD pid) : _main_td(nullptr), _mem_mgr(nullptr) {
					if (!pid) {
						_ntv_hdl = nullptr;
						_need_close = false;
						return;
					}
					_ntv_hdl = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
					_need_close = true;
					if (*this != process::from_this()) {
						_mem_mgr = mem_mgr_t(*this);
					}
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

				process(process &&src) : _ntv_hdl(src._ntv_hdl), _main_td(src._main_td), _need_close(src._need_close), _mem_mgr(src._mem_mgr) {
					if (src) {
						src._ntv_hdl = nullptr;
						src._main_td = nullptr;
						src._need_close = false;
						src._mem_mgr = nullptr;
					}
				}

				process &operator=(process &&src) {
					reset();

					if (src) {
						_ntv_hdl = src._ntv_hdl;
						_main_td = src._main_td;
						_need_close = src._need_close;
						_mem_mgr = src._mem_mgr;
						src._ntv_hdl = nullptr;
						src._main_td = nullptr;
						src._need_close = false;
						src._mem_mgr = nullptr;
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

				HMODULE native_module_handle(const std::string &name = "") {
					HMODULE mdu;
					if (*this == process::from_this()) {
						mdu = GetModuleHandleW(name.empty() ? nullptr : u8_to_w(name).c_str());
					} else {
						if (name.empty()) {
							mdu = syscall(&GetModuleHandleW, nullptr);
						} else {
							mdu = syscall(&GetModuleHandleW, u8_to_w(name));
						}
					}
					return mdu;
				}

				std::string file_path(HMODULE mdu = nullptr) {
					if (!mdu) {
						mdu = native_module_handle();
					}
					WCHAR path[MAX_PATH];
					GetModuleFileNameExW(_ntv_hdl, mdu, path, MAX_PATH);
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

				class mem_mgr_t_c : public virtual io::allocator_c, public virtual io::read_writer_at_c {
					public:
						mem_mgr_t_c(const process &proc) : _ph(proc.native_handle()) {}

						virtual ~mem_mgr_t_c() {}

						virtual any_ptr alloc(size_t size) {
							return VirtualAllocEx(_ph, nullptr, size, MEM_COMMIT, PAGE_READWRITE);
						}

						virtual void free(any_ptr ptr) {
							VirtualFreeEx(_ph, ptr, 0, MEM_RELEASE);
						}

						virtual mem::data read_at(ptrdiff_t pos, size_t size = static_cast<size_t>(-1)) const {
							mem::data cache(size);
							SIZE_T sz;
							ReadProcessMemory(_ph, any_ptr(pos), cache.base(), cache.size(), &sz);
							return cache.slice(0, sz);
						}

						virtual size_t write_at(ptrdiff_t pos, const mem::data &dat) {
							SIZE_T sz;
							WriteProcessMemory(_ph, any_ptr(pos), dat.base(), dat.size(), &sz);
							return sz;
						}

					private:
						HANDLE _ph;
				};

				using mem_mgr_t = obj<mem_mgr_t_c>;

				mem_mgr_t mem_mgr() {
					return _mem_mgr;
				}

				io::data mem_data(any_ptr base, size_t size = 0) {
					return io::data(base, size, _mem_mgr, _mem_mgr);
				}

				io::data mem_image(HMODULE mdu = nullptr) {
					if (!mdu) {
						mdu = native_module_handle();
					}
					MODULEINFO mi;
					GetModuleInformation(_ntv_hdl, mdu, &mi, sizeof(MODULEINFO));
					return mem_data(mi.lpBaseOfDll, mi.SizeOfImage);
				}

				io::data mem_data(size_t size) {
					return io::data(size, _mem_mgr, _mem_mgr);
				}

				io::data mem_data(const std::string &str) {
					auto sz = str.length() + 1;
					auto md = mem_data(sz);
					md.copy(io::data(str.c_str(), sz));
					return std::move(md);
				}

				io::data mem_data(const std::wstring &wstr) {
					auto byt_sz = (wstr.length() + 1) * sizeof(wchar_t);
					auto md = mem_data(byt_sz);
					md.copy(io::data(wstr.c_str(), byt_sz));
					return std::move(md);
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

				any_word syscall(any_ptr func, std::nullptr_t) {
					return syscall(func);
				}

				any_word syscall(any_ptr func, const std::string &param) {
					return syscall(func, any_word(mem_data(param).base()));
				}

				any_word syscall(any_ptr func, const std::wstring &param) {
					return syscall(func, any_word(mem_data(param).base()));
				}

				any_word syscall(any_ptr func, const char *param) {
					return syscall(func, std::string(param));
				}

				any_word syscall(any_ptr func, const wchar_t *param) {
					return syscall(func, std::wstring(param));
				}

			private:
				HANDLE _ntv_hdl, _main_td;
				bool _need_close;
				mem_mgr_t _mem_mgr;
		};
	#endif
}

#endif
