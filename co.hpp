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

	class continuation {
		public:
			typedef
			#if defined(_WIN32)
				LPVOID
			#elif defined(_TMD_UNIX_)
				ucontext_t
			#endif
			native_handle_t;

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

		private:
			native_handle_t _ntv_hdl;
	};

	class coroutine {
		public:
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
					_oc.native_handle() = CreateFiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<PVOID>(_func)
					);
				#elif defined(_TMD_UNIX_)
					getcontext(&_oc.native_handle());
					_oc.native_handle().uc_stack.ss_sp = reinterpret_cast<decltype(_oc.native_handle().uc_stack.ss_sp)>(new uint8_t[stack_size]);
					_oc.native_handle().uc_stack.ss_size = stack_size;
					_oc.native_handle().uc_stack.ss_flags = 0;
					_oc.native_handle().uc_link = nullptr;
					makecontext(&_oc.native_handle(), &_fiber_func_shell, reinterpret_cast<void *>(_func));
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
				_oc.join();
				_executed = true;
			}

			void execute(continuation &cc_receiver) {
				assert(executable());
				_oc.join(cc_receiver);
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
					_oc.native_handle() = src._oc.native_handle();
				#elif defined(_TMD_UNIX_)
					if (joined) {
						_oc.native_handle().uc_stack.ss_sp = src._oc.native_handle().uc_stack.ss_sp;
					} else {
						_oc.native_handle() = src._oc.native_handle();
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
						_oc.native_handle() = src._oc.native_handle();
					#elif defined(_TMD_UNIX_)
						if (joined) {
							_oc.native_handle().uc_stack.ss_sp = src._oc.native_handle().uc_stack.ss_sp;
						} else {
							_oc.native_handle() = src._oc.native_handle();
						}
					#endif

					src._func = nullptr;
				}

				return *this;
			}

		private:
			std::function<void()> *_func;
			bool _executed;
			continuation _oc;

			void _dtor() {
				if (_func) {
					#if defined(_WIN32)
						DeleteFiber(_oc.native_handle());
					#elif defined(_TMD_UNIX_)
						delete[] reinterpret_cast<uint8_t *>(_oc.native_handle().uc_stack.ss_sp);
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
	};
}

#endif
