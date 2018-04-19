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
				using id_t = native_handle_t;

				constexpr coro_joiner(native_handle_t fiber = nullptr) : _fiber(fiber) {}

				coro_joiner(const coro_joiner &) = default;

				coro_joiner &operator=(const coro_joiner &) = default;

				coro_joiner(coro_joiner &&src) : _fiber(src._fiber) {
					if (src._fiber) {
						src.reset();
					}
				}

				coro_joiner &operator=(coro_joiner &&src) {
					if (src._fiber) {
						_fiber = src._fiber;
						src.reset();
					} else {
						reset();
					}
					return *this;
				}

				native_handle_t native_handle() const {
					return _fiber;
				}

				id_t id() const {
					return _fiber;
				}

				operator bool() const {
					return _fiber;
				}

				void join() const {
					#ifndef NDEBUG
						auto this_fiber = GetCurrentFiber();
					#endif
					assert(this_fiber && reinterpret_cast<uintptr_t>(this_fiber) != 0x1E00);
					assert(_fiber != this_fiber);

					SwitchToFiber(_fiber);
				}

				void operator()() const {
					return join();
				}

				void reset() {
					_fiber = nullptr;
				}

			private:
				native_handle_t _fiber;
		};

		class coro : public coro_joiner {
			public:
				static constexpr size_t default_stack_size = 0;

				constexpr coro() : coro_joiner(), _func() {}

				coro(std::function<void()> func, size_t stack_size = default_stack_size) : _func(new decltype(func)(std::move(func))) {
					reinterpret_cast<coro_joiner &>(*this) = _new_fiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func),
						reinterpret_cast<LPVOID>(_func.get())
					);
				}

				coro(const coro &) = delete;

				coro &operator=(const coro &) = delete;

				coro(coro &&src) : _func(std::move(src._func)) {}

				coro &operator=(coro &&src) {
					reset();
					if (src) {
						reinterpret_cast<coro_joiner &>(*this) = std::move(src);
						_func = std::move(src._func);
					}
					return *this;
				}

				void reset() {
					_func.reset();
				}

			protected:
				std::unique_ptr<std::function<void()>> _func;

				static void WINAPI _fiber_func(std::function<void()> *_func) {
					(*_func)();
				}
		};

		namespace this_coro {
			inline coro::native_handle_t native_handle() {
				return _this_fiber();
			}

			inline coro::id_t id() {
				return native_handle();
			}

			inline coro_joiner joiner() {
				return native_handle();
			}
		};
	}
}

#endif
