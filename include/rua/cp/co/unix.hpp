#ifndef _RUA_CP_CO_UNIX_HPP
#define _RUA_CP_CO_UNIX_HPP

#include "../../macros.hpp"

#ifndef RUA_UNIX
	#error rua::co: not supported this platform!
#endif

#if defined(RUA_MAC)
	#include <sys/ucontext.h>
	#include <sys/unistd.h>
#else
	#include <ucontext.h>
	#include <unistd.h>
#endif

#include <pthread.h>
#include <cstdlib>

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace rua {
	namespace cp {
		class cont {
			public:
				using native_handle_s = ucontext_t;
				using native_handle_t = const native_handle_s &;
				using native_resource_handle_t = void *;

				constexpr cont() : _joinable(false) {
					_ntv_hdl.uc_stack.ss_sp = nullptr;
				}

				cont(const cont &) = delete;

				cont(cont &&src) : _ntv_hdl(src._ntv_hdl), _joinable(src._joinable) {
					src.reset();
				}

				cont &operator=(const cont &) = delete;

				cont &operator=(cont &&src) {
					_ntv_hdl = src._ntv_hdl;
					_joinable = src._joinable;
					src.reset();
					return *this;
				}

				native_handle_t native_handle() const {
					return _ntv_hdl;
				}

				native_resource_handle_t native_resource_handle() const {
					return _ntv_hdl.uc_stack.ss_sp;
				}

				bool joinable() const {
					return _joinable && native_resource_handle();
				}

				operator bool() const {
					return joinable();
				}

				void join() {
					assert(joinable());

					_joinable = false;
					setcontext(&_ntv_hdl);
				}

				void join(cont &cc_receiver) {
					assert(joinable());

					_joinable = false;
					cc_receiver._joinable = true;

					swapcontext(&cc_receiver._ntv_hdl, &_ntv_hdl);
				}

				void reset() {
					_ntv_hdl.uc_stack.ss_sp = nullptr;
					_joinable = false;
				}

			protected:
				native_handle_s _ntv_hdl;
				bool _joinable;
		};

		class coro : public cont {
			public:
				static constexpr size_t default_stack_size = 8 * 1024;

				constexpr coro() : _func(nullptr) {}

				coro(const std::function<void()> &func, size_t stack_size = default_stack_size) : _func(new std::function<void()>(func)) {
					_joinable = true;

					getcontext(&_ntv_hdl);
					_ntv_hdl.uc_stack.ss_sp = reinterpret_cast<void *>(new uint8_t[stack_size]);
					_ntv_hdl.uc_stack.ss_size = stack_size;
					_ntv_hdl.uc_stack.ss_flags = 0;
					_ntv_hdl.uc_link = nullptr;
					makecontext(&_ntv_hdl, &_fiber_func_shell, reinterpret_cast<void *>(_func));
				}

				coro(const coro &) = delete;

				coro(coro &&src) : cont(static_cast<cont &&>(src)) {
					if (!src._func) {
						_func = nullptr;
						return;
					}
					src._func = nullptr;
				}

				coro &operator=(const coro &) = delete;

				coro &operator=(coro &&src) {
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
						delete[] reinterpret_cast<uint8_t *>(native_resource_handle());
						delete _func;
						_func = nullptr;
						cont::reset();
					}
				}

				void reset() = delete;

			private:
				std::function<void()> *_func;

				static void _fiber_func_shell(std::function<void()> *func) {
					(*func)();
				}
		};
	}
}

#endif
