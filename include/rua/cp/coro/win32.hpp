#ifndef _RUA_CP_CORO_WIN32_HPP
#define _RUA_CP_CORO_WIN32_HPP

#ifndef _WIN32
	#error rua::cp::coro: not supported this platform!
#endif

#include <windows.h>

#include <functional>
#include <atomic>
#include <memory>
#include <string>
#include <cassert>

namespace rua {
	namespace cp {
		using _fiber_t = LPVOID;

		inline _fiber_t _this_fiber() {
			auto fiber = GetCurrentFiber();
			if (!fiber || reinterpret_cast<uintptr_t>(fiber) == 0x1E00) {
				static auto dl_ConvertThreadToFiberEx =
					reinterpret_cast<decltype(&ConvertThreadToFiberEx)>(
						GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "ConvertThreadToFiberEx")
					)
				;
				return
					dl_ConvertThreadToFiberEx ?
					dl_ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH) :
					ConvertThreadToFiber(nullptr)
				;
			}
			return fiber;
		}

		inline _fiber_t _new_fiber(SIZE_T dwStackSize, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter) {
			static auto dl_CreateFiberEx =
				reinterpret_cast<decltype(&CreateFiberEx)>(
					GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFiberEx")
				)
			;
			return
				dl_CreateFiberEx ?
				dl_CreateFiberEx(dwStackSize, dwStackSize, FIBER_FLAG_FLOAT_SWITCH, lpStartAddress, lpParameter) :
				CreateFiber(dwStackSize, lpStartAddress, lpParameter)
			;
		}

		class coro_joiner {
			public:
				using native_handle_t = _fiber_t;

				constexpr coro_joiner(native_handle_t fiber = nullptr) : _fiber(fiber), _joinable(fiber ? true : false) {}

				~coro_joiner() {
					reset();
				}

				coro_joiner(const coro_joiner &) = default;

				coro_joiner &operator=(const coro_joiner &) = default;

				coro_joiner(coro_joiner &&src) : _fiber(src._fiber), _joinable(src._joinable) {
					if (src) {
						src.reset();
					}
				}

				coro_joiner &operator=(coro_joiner &&src) {
					if (src) {
						_fiber = src._fiber;
						_joinable = true;
						src.reset();
					} else {
						reset();
					}
					return *this;
				}

				native_handle_t native_handle() const {
					return _fiber;
				}

				bool joinable() const {
					return _joinable;
				}

				operator bool() const {
					return _joinable;
				}

				void join() {
					assert(_joinable);

					_joinable = false;
					_this_fiber();
					SwitchToFiber(_fiber);
				}

				void join(coro_joiner &get_cur) {
					assert(_joinable);

					_joinable = false;
					get_cur = _this_fiber();
					SwitchToFiber(_fiber);
				}

				template <typename... A>
				void operator()(A&&... a) const {
					return join(std::forward<A>(a)...);
				}

				void reset() {
					if (!*this) {
						return;
					}
					_fiber = nullptr;
					_joinable = false;
				}

			private:
				native_handle_t _fiber;
				bool _joinable;
		};

		class coro : public coro_joiner {
			public:
				using joiner_t = coro_joiner;

				static constexpr size_t default_stack_size = 0;

				constexpr coro() : coro_joiner(), _func() {}

				coro(std::function<void()> func, size_t stack_size = default_stack_size) : _func(new decltype(func)(std::move(func))) {
					reinterpret_cast<coro_joiner &>(*this) = _new_fiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func),
						reinterpret_cast<LPVOID>(_func.get())
					);
				}

				~coro() {
					reset();
				}

				coro(const coro &) = delete;

				coro &operator=(const coro &) = delete;

				coro(coro &&src) = default;

				coro &operator=(coro &&src) = default;

				operator bool() const {
					return _func.get();
				}

				void reset() {
					DeleteFiber(native_handle());
					coro_joiner::reset();
					_func.reset();
				}

			protected:
				std::unique_ptr<std::function<void()>> _func;

				static void WINAPI _fiber_func(std::function<void()> *_func) {
					(*_func)();
					exit(0);
				}
		};
	}
}

#endif
