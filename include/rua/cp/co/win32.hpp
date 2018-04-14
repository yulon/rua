#ifndef _RUA_CP_CO_WIN32_HPP
#define _RUA_CP_CO_WIN32_HPP

#ifndef _WIN32
	#error rua::co: not supported this platform!
#endif

#include <windows.h>

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace rua {
	namespace cp {
		class cont {
			public:
				using native_handle_s = LPVOID;
				using native_handle_t = native_handle_s;
				using native_resource_handle_t = LPVOID;

				constexpr cont() : _ntv_hdl(nullptr), _joinable(false) {}

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
					return _ntv_hdl;
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
					_touch_cur_fiber();
					SwitchToFiber(_ntv_hdl);
				}

				void join(cont &cc_receiver) {
					assert(joinable());

					_joinable = false;
					cc_receiver._joinable = true;

					cc_receiver._ntv_hdl = _touch_cur_fiber();
					SwitchToFiber(_ntv_hdl);
				}

				void reset() {
					_ntv_hdl = nullptr;
					_joinable = false;
				}

			protected:
				native_handle_s _ntv_hdl;
				bool _joinable;

				static LPVOID _touch_cur_fiber() {
					auto cur = GetCurrentFiber();
					if (!cur || reinterpret_cast<uintptr_t>(cur) == 0x1E00) {
						return
							#ifdef RUA_WINXP_SUPPORT
								ConvertThreadToFiber(nullptr)
							#else
								ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH)
							#endif
						;
					}
					return cur;
				}
		};

		class coro : public cont {
			public:
				static constexpr size_t default_stack_size = 0;

				constexpr coro() : _func(nullptr) {}

				coro(const std::function<void()> &func, size_t stack_size = default_stack_size) : _func(new std::function<void()>(func)) {
					_joinable = true;

					_ntv_hdl = _create_fiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<LPVOID>(_func)
					);
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
						DeleteFiber(native_resource_handle());
						delete _func;
						_func = nullptr;
						cont::reset();
					}
				}

				void reset() = delete;

			private:
				std::function<void()> *_func;

				static void WINAPI _fiber_func_shell(std::function<void()> *func) {
					(*func)();
				}

				static LPVOID _create_fiber(SIZE_T dwStackSize, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter) {
					return
						#ifdef RUA_WINXP_SUPPORT
							CreateFiber(dwStackSize, lpStartAddress, lpParameter)
						#else
							CreateFiberEx(dwStackSize, dwStackSize, FIBER_FLAG_FLOAT_SWITCH, lpStartAddress, lpParameter)
						#endif
					;
				}
		};
	}
}

#endif
