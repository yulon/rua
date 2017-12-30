#ifndef _TMD_PROCESS_HPP
#define _TMD_PROCESS_HPP

#include "bin.hpp"
#include "unsafe_ptr.hpp"

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
					std::wstring wname(u8_to_u16(name));
					for (;;) {
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
						std::this_thread::sleep_for(std::chrono::milliseconds(500));
					}
				}

				static process from_this() {
					return GetCurrentProcess();
				}

				////////////////////////////////////////////////////////////////

				constexpr process() : _ntv_hdl(nullptr), _main_td(nullptr), _need_close(false) {}

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

					std::wstring file_w(u8_to_u16(file));
					std::wstringstream cmd;
					if (args.size()) {
						cmd << "\"" << file_w << "\"";
						for (auto &arg : args) {
							cmd << " \"" << u8_to_u16(arg) << "\"";
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
						pwd.empty() ? nullptr : u8_to_u16(pwd).c_str(),
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

				process(DWORD pid) : _ntv_hdl(
					OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)
				), _main_td(nullptr), _need_close(true) {}

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

				bool operator==(const process &target) {
					return _ntv_hdl == target._ntv_hdl;
				}

				bool operator!=(const process &target) {
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

				HMODULE native_module_handle(const std::string &name = "") {
					HMODULE mdu;
					if (*this == process::from_this()) {
						mdu = GetModuleHandleW(name.empty() ? nullptr : u8_to_u16(name).c_str());
					} else {
						if (name.empty()) {
							mdu = syscall(&GetModuleHandleW, nullptr);
						} else {
							mdu = syscall(&GetModuleHandleW, u8_to_u16(name));
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
					return u16_to_u8(path);
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

				bin::allocator_t mem_allocator() {
					return *this == process::from_this() ? bin::allocator_t{nullptr, nullptr} : bin::allocator_t{
						[this](size_t size)->unsafe_ptr {
							return VirtualAllocEx(_ntv_hdl, nullptr, size, MEM_COMMIT, PAGE_READWRITE);
						},
						[this](unsafe_ptr ptr, size_t size) {
							VirtualFreeEx(_ntv_hdl, ptr, size, MEM_COMMIT);
						}
					};
				}

				bin_ref::read_writer_t mem_read_writer() {
					return *this == process::from_this() ? bin_ref::read_writer_t{nullptr, nullptr} : bin_ref::read_writer_t{
						[this](unsafe_ptr src, unsafe_ptr data, size_t size) {
							SIZE_T writed_len;
							ReadProcessMemory(_ntv_hdl, src, data, size, &writed_len);
						},
						[this](unsafe_ptr dest, unsafe_ptr data, size_t size) {
							SIZE_T writed_len;
							WriteProcessMemory(_ntv_hdl, dest, data, size, &writed_len);
						}
					};
				}

				bin mem_alloc(size_t size) {
					return bin(size, mem_allocator(), mem_read_writer());
				}

				bin mem_alloc(const std::string &str) {
					return bin(str.c_str(), str.length() + 1, mem_allocator(), mem_read_writer());
				}

				bin mem_alloc(const std::wstring &wstr) {
					return bin(wstr.c_str(), (wstr.length() + 1) * sizeof(wchar_t), mem_allocator(), mem_read_writer());
				}

				bin_ref mem_ref(unsafe_ptr ptr, size_t size) {
					return bin_ref(ptr, size, mem_read_writer());
				}

				template <typename F>
				unsafe_ptr syscall(F func, unsafe_ptr param) {
					HANDLE td;
					DWORD tid;
					td = CreateRemoteThread(_ntv_hdl, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(func), param, 0, &tid);
					if (!td) {
						return 0;
					}
					WaitForSingleObject(td, INFINITE);
					DWORD result;
					GetExitCodeThread(td, &result);
					CloseHandle(td);
					return result;
				}

				template <typename F>
				unsafe_ptr syscall(F func, std::nullptr_t) {
					return syscall(func, unsafe_ptr(nullptr));
				}

				template <typename F>
				unsafe_ptr syscall(F func, const std::string &param) {
					return syscall(func, mem_alloc(param).data());
				}

				template <typename F>
				unsafe_ptr syscall(F func, const std::wstring &param) {
					return syscall(func, mem_alloc(param).data());
				}

				template <typename F>
				unsafe_ptr syscall(F func, const char *param) {
					return syscall(func, std::string(param));
				}

				template <typename F>
				unsafe_ptr syscall(F func, const wchar_t *param) {
					return syscall(func, std::wstring(param));
				}

				bin_ref mem_ref(HMODULE mdu = nullptr) {
					if (!mdu) {
						mdu = native_module_handle();
					}
					MODULEINFO mi;
					GetModuleInformation(_ntv_hdl, mdu, &mi, sizeof(MODULEINFO));
					return bin_ref(mi.lpBaseOfDll, mi.SizeOfImage, mem_read_writer());
				}

			private:
				HANDLE _ntv_hdl, _main_td;
				bool _need_close;
		};
	#endif
}

#endif
