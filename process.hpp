#ifndef _TMD_PROCESS_HPP
#define _TMD_PROCESS_HPP

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cassert>

namespace tmd {
	#ifdef _WIN32
		class process {
			public:
				constexpr process() : _ntv_hdl(nullptr), _main_td(nullptr), _need_close(false) {}

				process(HANDLE proc) : _ntv_hdl(proc), _main_td(nullptr), _need_close(false) {}

				process(
					const std::string &file,
					const std::vector<std::string> &args,
					const std::string &pwd,
					bool pause_main_thread = false
				) : _need_close(false) {
					STARTUPINFOA si;
					PROCESS_INFORMATION pi;
					memset(&si, 0, sizeof(si));
					memset(&pi, 0, sizeof(pi));
					si.cb = sizeof(si);

					std::stringstream cmd;
					if (args.size()) {
						cmd << "\"" << file << "\"";
						for (auto &arg : args) {
							cmd << " \"" << arg << "\"";
						}
					}

					if (!CreateProcessA(
						file.c_str(),
						args.size() ? const_cast<char *>(cmd.str().c_str()) : nullptr,
						nullptr,
						nullptr,
						true,
						pause_main_thread ? CREATE_SUSPENDED : 0,
						nullptr,
						pwd.empty() ? nullptr : pwd.c_str(),
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
					bool pause_main_thread = false
				) : process(file, {}, "", pause_main_thread) {}

				process(DWORD pid) : _ntv_hdl(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid)), _main_td(nullptr), _need_close(true) {}

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

				class mem_t {
					public:
						constexpr mem_t() : _proc(nullptr), _ptr(nullptr), _sz(0) {}

						mem_t(const process &proc, size_t size) :
							_proc(&proc),
							_ptr(VirtualAllocEx(proc, NULL, size, MEM_COMMIT, PAGE_READWRITE)),
							_sz(size)
						{}

						mem_t(const process &proc, const std::string &str) : mem_t(proc, str.length() + 1) {
							if (!write(reinterpret_cast<const uint8_t *>(str.c_str()))) {
								free();
							}
						}

						~mem_t() {
							free();
						}

						LPVOID native_handle() const {
							return _ptr;
						}

						operator LPVOID() const {
							return native_handle();
						}

						operator bool() {
							return native_handle();
						}

						mem_t(const mem_t &) = delete;

						mem_t &operator=(const mem_t &) = delete;

						mem_t(mem_t &&src) : _proc(src._proc), _ptr(src._ptr) {
							if (src) {
								src._ptr = nullptr;
							}
						}

						mem_t &operator=(mem_t &&src) {
							free();

							if (src) {
								_proc = src._proc;
								_ptr = src._ptr;
								src._ptr = nullptr;
							}
						}

						bool write(const uint8_t *data, size_t size = 0) {
							assert(_ptr);

							if (!size) {
								size = _sz;
							}

							SIZE_T writed_len;
							if (!WriteProcessMemory(*_proc, _ptr, data, size, &writed_len)) {
								return false;
							}
							if (writed_len != size) {
								return false;
							}
							return true;
						}

						void free() {
							if (_ptr) {
								VirtualFreeEx(*_proc, _ptr, _sz, MEM_COMMIT);
								_ptr = nullptr;
							}
						}

					private:
						const process *_proc;
						LPVOID _ptr;
						size_t _sz;
				};

				template <typename... A>
				mem_t alloc(A&&... a) {
					return mem_t(_ntv_hdl, std::forward<A>(a)...);
				}

				template <typename F>
				DWORD syscall(F func, LPVOID param) {
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
				DWORD syscall(F func, const std::string &param) {
					return syscall(_ntv_hdl, func, alloc(param));
				}

			private:
				HANDLE _ntv_hdl, _main_td;
				bool _need_close;
		};
	#endif
}

#endif
