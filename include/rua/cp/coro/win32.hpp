#ifndef _RUA_CP_CORO_WIN32_HPP
#define _RUA_CP_CORO_WIN32_HPP

#ifndef _WIN32
	#error rua::cp::coro: not supported this platform!
#endif

#include "../tls/win32.hpp"

#include <windows.h>

#include <functional>
#include <atomic>
#include <queue>
#include <mutex>
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

		class coro {
			public:
				static coro from_this_thread() {
					_ctx_t *cur_ctx;
					if (fls::valid()) {
						cur_ctx = _fls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{2},
								{true},
								{false},
								_this_fiber(),
								nullptr,
								nullptr
							};
							_fls_ctx().set(cur_ctx);
						} else {
							++cur_ctx->use_count;
						}
					} else {
						cur_ctx = _tls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{2},
								{true},
								{false},
								_this_fiber(),
								nullptr,
								nullptr
							};
							_tls_ctx().set(cur_ctx);
						} else {
							++cur_ctx->use_count;
						}
					}
					return cur_ctx;
				}

				using native_handle_t = _fiber_t;
				using id_t = void *;

				constexpr coro() : _ctx(nullptr) {}

				static constexpr size_t default_stack_size = 0;

				coro(std::function<void()> start, size_t stack_size = default_stack_size) : _ctx(
					new _ctx_t{
						{1},
						{true},
						{false},
						nullptr,
						std::move(start),
						nullptr
					}
				) {
					_ctx->fiber = _new_fiber(
						stack_size,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func),
						reinterpret_cast<LPVOID>(_ctx)
					);
				}

				~coro() {
					reset();
				}

				coro(const coro &src) : _ctx(src._ctx) {
					if (_ctx) {
						++_ctx->use_count;
					}
				}

				coro &operator=(const coro &src) {
					reset();
					if (src._ctx) {
						++src._ctx->use_count;
						_ctx = src._ctx;
					}
					return *this;
				}

				coro(coro &&src) : _ctx(src._ctx) {
					if (src._ctx) {
						src._ctx = nullptr;
					}
				}

				coro &operator=(coro &&src) {
					reset();
					if (src._ctx) {
						_ctx = src._ctx;
						src._ctx = nullptr;
					}
					return *this;
				}

				operator bool() const {
					return _ctx;
				}

				native_handle_t native_handle() const {
					return _ctx->fiber;
				}

				id_t id() const {
					return _ctx;
				}

				bool joinable() const {
					return _ctx->joinable;
				}

				bool join() {
					if (!_ctx->joinable.exchange(false)) {
						return false;
					}
					++_ctx->use_count;
					_join();
					return true;
				}

				bool operator()() {
					return join();
				}

				bool join_and_detach() {
					if (!_ctx->joinable.exchange(false)) {
						return false;
					}
					_join();
					_ctx = nullptr;
					return true;
				}

				void reset() {
					if (!_ctx) {
						return;
					}
					if (!--_ctx) {
						if (!--_ctx->use_count) {
							_del(_ctx);
						}
					}
					_ctx = nullptr;
				}

			private:
				struct _ctx_t {
					std::atomic<size_t> use_count;
					std::atomic<bool> joinable, exited;
					native_handle_t fiber;
					std::function<void()> start;
					_ctx_t *joiner;

					void handle_joiner() {
						if (!joiner) {
							return;
						}
						if (!joiner->exited) {
							joiner->joinable = true;
						} else if (!--joiner->use_count) {
							_del(joiner);
							joiner = nullptr;
						}
					}
				};

				_ctx_t *_ctx;

				coro(_ctx_t *ctx) : _ctx(ctx) {}

				void _join() {
					_ctx_t *cur_ctx;

					if (fls::valid()) {
						cur_ctx = _fls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{1},
								{false},
								{false},
								_this_fiber(),
								nullptr,
								nullptr
							};
						}
					} else {
						cur_ctx = _tls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{1},
								{false},
								{false},
								_this_fiber(),
								nullptr,
								nullptr
							};
						}
						_tls_ctx().set(_ctx);
					}

					// A cur_ctx->use_count ownership form cur_ctx move to _ctx->joiner.
					_ctx->joiner = cur_ctx;
					SwitchToFiber(_ctx->fiber);

					// on back
					cur_ctx->handle_joiner();
				}

				static void _del(_ctx_t *_ctx) {
					DeleteFiber(_ctx->fiber);
					delete _ctx;
				}

				static tls &_tls_ctx() {
					static tls inst;
					return inst;
				}

				static fls &_fls_ctx() {
					static fls inst;
					return inst;
				}

				static void WINAPI _fiber_func(_ctx_t *cur_ctx) {
					if (fls::valid()) {
						_fls_ctx().set(cur_ctx);
					}
					cur_ctx->handle_joiner();
					cur_ctx->start();
					cur_ctx->exited = true;
					if (cur_ctx->joiner && cur_ctx->joinable.exchange(false)) {
						// A cur_ctx->use_count ownership form cur_ctx move to cur_ctx->joiner->joiner.
						cur_ctx->joiner->joiner = cur_ctx;
						SwitchToFiber(cur_ctx->joiner);
					}
					if (!--cur_ctx->use_count) {
						// DeleteFiber(cur_ctx->fiber) by OS
						delete cur_ctx;
					}
				}
		};
	}
}

#endif
