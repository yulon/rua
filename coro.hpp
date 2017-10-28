#ifndef _TMD_CORO_HPP
#define _TMD_CORO_HPP

#if defined(_WIN32)
	#include <windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	#ifndef _TMD_UNIX_
		#define _TMD_UNIX_ 1
	#endif

	#if defined(__APPLE__) && defined(__MACH__)
		#include <sys/ucontext.h>
		#include <sys/unistd.h>
	#else
		#include <ucontext.h>
		#include <unistd.h>
	#endif

	#include <pthread.h>
	#include <cstdlib>
#else
	#error tmd::coro: not support this OS!
#endif

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace tmd {
	class coro {
		public:
			class cont {
				public:
					typedef
					#if defined(_WIN32)
						LPVOID
					#elif defined(_TMD_UNIX_)
						ucontext_t
					#endif
					native_handle_t;

					cont(std::nullptr_t np = nullptr)
						#if defined(_WIN32)
							: _ntv_hdl(np)
						#endif
					{
						#if defined(_TMD_UNIX_)
							_ntv_hdl.uc_stack.ss_sp = np;
						#endif
					}

					operator bool() {
						#if defined(_WIN32)
							return _ntv_hdl;
						#elif defined(_TMD_UNIX_)
							return _ntv_hdl.uc_stack.ss_sp;
						#endif
					}

					std::nullptr_t operator=(std::nullptr_t np) {
						#if defined(_WIN32)
							_ntv_hdl
						#elif defined(_TMD_UNIX_)
							_ntv_hdl.uc_stack.ss_sp
						#endif
						= np;

						return np;
					}

					native_handle_t &native_handle() {
						return _ntv_hdl;
					}

					const native_handle_t &native_handle() const {
						return _ntv_hdl;
					}

					void join() {
						#if defined(_WIN32)
							SwitchToFiber(_ntv_hdl);
						#elif defined(_TMD_UNIX_)
							setcontext(_ntv_hdl);
						#endif
					}

					void join(cont &cc_receiver) {
						#if defined(_WIN32)
							cc_receiver._ntv_hdl = GetCurrentFiber();
							SwitchToFiber(_ntv_hdl);
						#elif defined(_TMD_UNIX_)
							swapcontext(&cc_receiver._ntv_hdl, native_handle());
						#endif
					}

					bool belong_to(const coro &co) {
						#if defined(_WIN32)
							if (_ntv_hdl == co._start_ct._ntv_hdl) {
								return true;
							}
						#elif defined(_TMD_UNIX_)
							if (_ntv_hdl.uc_stack.ss_sp == co._start_ct._ntv_hdl.uc_stack.ss_sp) {
								return true;
							}
						#endif

						return false;
					}

				private:
					native_handle_t _ntv_hdl;
			};

			////////////////////////////////////////////////////////////////////

			static constexpr size_t default_stack_size =
				#if defined(_WIN32)
					0
				#elif defined(_TMD_UNIX_)
					8 * 1024
				#endif
			;

			coro() : _func(nullptr) {}

			coro(const std::function<void()> &func, size_t stack_size = default_stack_size) : _func(new std::function<void()>(func)) {
				_executed = false;

				#if defined(_WIN32)
					_start_ct.native_handle() = CreateFiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<PVOID>(_func)
					);
				#elif defined(_TMD_UNIX_)
					getcontext(&_start_ct.native_handle());
					_start_ct.native_handle().uc_stack.ss_sp = reinterpret_cast<decltype(_start_ct.native_handle().uc_stack.ss_sp)>(new uint8_t[stack_size]);
					_start_ct.native_handle().uc_stack.ss_size = stack_size;
					_start_ct.native_handle().uc_stack.ss_flags = 0;
					_start_ct.native_handle().uc_link = nullptr;
					makecontext(&_start_ct.native_handle(), &_fiber_func_shell, reinterpret_cast<void *>(_func));
				#endif
			}

			~coro() {
				_dtor();
			}

			operator bool() {
				return _func;
			}

			bool executable() {
				return _func && !_executed;
			}

			void execute() {
				assert(executable());

				_init();

				_start_ct.join();
				_executed = true;
			}

			void execute(cont &cc_receiver) {
				assert(executable());

				_init();

				_start_ct.join(cc_receiver);
				_executed = true;
			}

			coro(const coro &) = delete;

			coro(coro &&src) {
				if (!src._func) {
					_func = nullptr;
					return;
				}

				_func = src._func;
				_executed = src._executed;

				#if defined(_WIN32)
					_start_ct.native_handle() = src._start_ct.native_handle();
				#elif defined(_TMD_UNIX_)
					if (joined) {
						_start_ct.native_handle().uc_stack.ss_sp = src._start_ct.native_handle().uc_stack.ss_sp;
					} else {
						_start_ct.native_handle() = src._start_ct.native_handle();
					}
				#endif

				src._func = nullptr;
			}

			coro& operator=(const coro &) = delete;

			coro& operator=(coro &&src) {
				_dtor();

				if (src._func) {
					_func = src._func;
					_executed = src._executed;

					#if defined(_WIN32)
						_start_ct.native_handle() = src._start_ct.native_handle();
					#elif defined(_TMD_UNIX_)
						if (joined) {
							_start_ct.native_handle().uc_stack.ss_sp = src._start_ct.native_handle().uc_stack.ss_sp;
						} else {
							_start_ct.native_handle() = src._start_ct.native_handle();
						}
					#endif

					src._func = nullptr;
				}

				return *this;
			}

		private:
			std::function<void()> *_func;
			bool _executed;
			cont _start_ct;

			void _init() {
				#if defined(_WIN32)
					if (!GetCurrentFiber()) {
						auto fiber = ConvertThreadToFiber(nullptr);
						assert(fiber);
					}
				#endif
			}

			void _dtor() {
				if (_func) {
					#if defined(_WIN32)
						DeleteFiber(_start_ct.native_handle());
					#elif defined(_TMD_UNIX_)
						delete[] reinterpret_cast<uint8_t *>(_start_ct.native_handle().uc_stack.ss_sp);
					#endif

					delete _func;
					_func = nullptr;
				}
			}

			static void
				#if defined(_WIN32)
					WINAPI
				#endif
			_fiber_func_shell(std::function<void()> *func) {
				(*func)();
			}

			friend cont;
	};
}

#endif
