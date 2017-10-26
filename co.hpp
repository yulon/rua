#ifndef _TMD_CO_HPP
#define _TMD_CO_HPP

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
	#error tmd::co: not support this OS!
#endif

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace tmd {
	bool support_co_for_this_thread() {
		#if defined(_WIN32)
			if (!GetCurrentFiber()) {
				return ConvertThreadToFiber(nullptr);
			}
		#endif

		return true;
	}

	class coroutine {
		public:
			class continuation {
				public:
					typedef
					#if defined(_WIN32)
						LPVOID
					#elif defined(_TMD_UNIX_)
						ucontext_t
					#endif
					native_handle_t;

					continuation(std::nullptr_t np = nullptr)
						#if defined(_WIN32)
							: _ntv_hdl(nullptr)
						#endif
					{
						#if defined(_TMD_UNIX_)
							_ntv_hdl.uc_stack.ss_sp = nullptr;
						#endif
					}

					operator bool() {
						#if defined(_WIN32)
							return _ntv_hdl;
						#elif defined(_TMD_UNIX_)
							return _ntv_hdl.uc_stack.ss_sp;
						#endif
					}

					continuation &operator=(std::nullptr_t) {
						#if defined(_WIN32)
							_ntv_hdl
						#elif defined(_TMD_UNIX_)
							_ntv_hdl.uc_stack.ss_sp
						#endif
						= nullptr;
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

					void join(continuation &cc_receiver) {
						#if defined(_WIN32)
							cc_receiver._ntv_hdl = GetCurrentFiber();
							SwitchToFiber(_ntv_hdl);
						#elif defined(_TMD_UNIX_)
							swapcontext(&cc_receiver._ntv_hdl, native_handle());
						#endif
					}

					bool belong_to(const coroutine &cor) {
						#if defined(_WIN32)
							if (_ntv_hdl == cor._start_con._ntv_hdl) {
								return true;
							}
						#elif defined(_TMD_UNIX_)
							if (_ntv_hdl.uc_stack.ss_sp == cor._start_con._ntv_hdl.uc_stack.ss_sp) {
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

			coroutine() : _func(nullptr) {}

			coroutine(const std::function<void()> &func, size_t stack_size = default_stack_size) : _func(new std::function<void()>(func)) {
				_executed = false;

				#if defined(_WIN32)
					_start_con.native_handle() = CreateFiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<PVOID>(_func)
					);
				#elif defined(_TMD_UNIX_)
					getcontext(&_start_con.native_handle());
					_start_con.native_handle().uc_stack.ss_sp = reinterpret_cast<decltype(_start_con.native_handle().uc_stack.ss_sp)>(new uint8_t[stack_size]);
					_start_con.native_handle().uc_stack.ss_size = stack_size;
					_start_con.native_handle().uc_stack.ss_flags = 0;
					_start_con.native_handle().uc_link = nullptr;
					makecontext(&_start_con.native_handle(), &_fiber_func_shell, reinterpret_cast<void *>(_func));
				#endif
			}

			~coroutine() {
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
				_start_con.join();
				_executed = true;
			}

			void execute(continuation &cc_receiver) {
				assert(executable());
				_start_con.join(cc_receiver);
				_executed = true;
			}

			coroutine(const coroutine &) = delete;

			coroutine(coroutine &&src) {
				if (!src._func) {
					_func = nullptr;
					return;
				}

				_func = src._func;
				_executed = src._executed;

				#if defined(_WIN32)
					_start_con.native_handle() = src._start_con.native_handle();
				#elif defined(_TMD_UNIX_)
					if (joined) {
						_start_con.native_handle().uc_stack.ss_sp = src._start_con.native_handle().uc_stack.ss_sp;
					} else {
						_start_con.native_handle() = src._start_con.native_handle();
					}
				#endif

				src._func = nullptr;
			}

			coroutine& operator=(const coroutine &) = delete;

			coroutine& operator=(coroutine &&src) {
				_dtor();

				if (src._func) {
					_func = src._func;
					_executed = src._executed;

					#if defined(_WIN32)
						_start_con.native_handle() = src._start_con.native_handle();
					#elif defined(_TMD_UNIX_)
						if (joined) {
							_start_con.native_handle().uc_stack.ss_sp = src._start_con.native_handle().uc_stack.ss_sp;
						} else {
							_start_con.native_handle() = src._start_con.native_handle();
						}
					#endif

					src._func = nullptr;
				}

				return *this;
			}

		private:
			std::function<void()> *_func;
			bool _executed;
			continuation _start_con;

			void _dtor() {
				if (_func) {
					#if defined(_WIN32)
						DeleteFiber(_start_con.native_handle());
					#elif defined(_TMD_UNIX_)
						delete[] reinterpret_cast<uint8_t *>(_start_con.native_handle().uc_stack.ss_sp);
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

			friend continuation;
	};
}

#endif
