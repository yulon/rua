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
	#error tmd::coro: not support this OS!
#endif

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace tmd {
	class cont {
		public:
			typedef
				#if defined(_WIN32)
					LPVOID
				#elif defined(_TMD_UNIX_)
					ucontext_t
				#endif
			native_handle_s;

			typedef
				#if defined(_WIN32)
					native_handle_s
				#elif defined(_TMD_UNIX_)
					const native_handle_s &
				#endif
			native_handle_t;

			typedef
				#if defined(_WIN32)
					LPVOID
				#elif defined(_TMD_UNIX_)
					void *
				#endif
			native_resource_handle_t;

			cont() : _joinable(false) {}

			cont(const cont &) = delete;

			cont(cont &&src) : _ntv_hdl(src._ntv_hdl), _joinable(src._joinable) {
				src._joinable = false;
			}

			cont& operator=(const cont &) = delete;

			cont& operator=(cont &&src) {
				_ntv_hdl = src._ntv_hdl;
				_joinable = src._joinable;

				src._joinable = false;

				return *this;
			}

			bool joinable() const {
				#if defined(_WIN32)
					return _joinable && _ntv_hdl;
				#elif defined(_TMD_UNIX_)
					return _joinable && _ntv_hdl.uc_stack.ss_sp;
				#endif
			}

			operator bool() const {
				return joinable();
			}

			native_handle_t native_handle() const {
				return _ntv_hdl;
			}

			native_resource_handle_t native_resource_handle() {
				#if defined(_WIN32)
					return _ntv_hdl;
				#elif defined(_TMD_UNIX_)
					return _ntv_hdl.uc_stack.ss_sp;
				#endif
			}

			void join() {
				assert(joinable());

				_joinable = false;

				#if defined(_WIN32)
					_touch_cur_fiber();
					SwitchToFiber(_ntv_hdl);
				#elif defined(_TMD_UNIX_)
					setcontext(&_ntv_hdl);
				#endif
			}

			void join(cont &cc_receiver) {
				assert(joinable());

				_joinable = false;
				cc_receiver._joinable = true;

				#if defined(_WIN32)
					cc_receiver._ntv_hdl = _touch_cur_fiber();
					SwitchToFiber(_ntv_hdl);
				#elif defined(_TMD_UNIX_)
					swapcontext(&cc_receiver._ntv_hdl, &_ntv_hdl);
				#endif
			}

		protected:
			native_handle_s _ntv_hdl;
			bool _joinable;

			#if defined(_WIN32)
				static LPVOID _touch_cur_fiber() {
					auto cur = GetCurrentFiber();
					if (!cur) {
						return
							#ifdef TMD_WINXP_SUPPORT
								ConvertThreadToFiber(nullptr)
							#else
								ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH)
							#endif
						;
					}
					return cur;
				}
			#endif
	};

	class coro : public cont {
		public:
			static constexpr size_t default_stack_size =
				#if defined(_WIN32)
					0
				#elif defined(_TMD_UNIX_)
					8 * 1024
				#endif
			;

			coro() : _func(nullptr) {}

			coro(const std::function<void()> &func, size_t stack_size = default_stack_size) : _func(new std::function<void()>(func)) {
				_joinable = true;

				#if defined(_WIN32)
					_ntv_hdl = _create_fiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<LPVOID>(_func)
					);
				#elif defined(_TMD_UNIX_)
					getcontext(&_ntv_hdl);
					_ntv_hdl.uc_stack.ss_sp = reinterpret_cast<void *>(new uint8_t[stack_size]);
					_ntv_hdl.uc_stack.ss_size = stack_size;
					_ntv_hdl.uc_stack.ss_flags = 0;
					_ntv_hdl.uc_link = nullptr;
					makecontext(&_ntv_hdl, &_fiber_func_shell, reinterpret_cast<void *>(_func));
				#endif
			}

			coro(const coro &) = delete;

			coro(coro &&src) : cont(static_cast<cont &&>(src)) {
				if (!src._func) {
					_func = nullptr;
					return;
				}

				src._func = nullptr;
			}

			coro& operator=(const coro &) = delete;

			coro& operator=(coro &&src) {
				free();

				if (src._func) {
					static_cast<cont &>(*this) = static_cast<cont &&>(src);

					src._func = nullptr;
				}

				return *this;
			}

			~coro() {
				free();
			}

			operator bool() const {
				return _func;
			}

			void free() {
				if (_func) {
					#if defined(_WIN32)
						DeleteFiber
					#elif defined(_TMD_UNIX_)
						delete[] reinterpret_cast<uint8_t *>
					#endif
					(native_resource_handle());

					delete _func;
					_func = nullptr;
				}
			}

		private:
			std::function<void()> *_func;

			static void
				#if defined(_WIN32)
					WINAPI
				#endif
			_fiber_func_shell(std::function<void()> *func) {
				(*func)();
			}

			static LPVOID _create_fiber(SIZE_T dwStackSize, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter) {
				return
					#ifdef TMD_WINXP_SUPPORT
						CreateFiber(dwStackSize, lpStartAddress, lpParameter)
					#else
						CreateFiberEx(dwStackSize, dwStackSize, FIBER_FLAG_FLOAT_SWITCH, lpStartAddress, lpParameter)
					#endif
				;
			}
	};
}

#endif
