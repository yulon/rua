#ifndef _RUA_CP_CORO_UNI_HPP
#define _RUA_CP_CORO_UNI_HPP

#include "../../exec/cont.hpp"
#include "../tls.hpp"

#include <functional>
#include <atomic>
#include <memory>
#include <cassert>

namespace rua {
	namespace cp {
		namespace uni {
			class coro {
				public:
					static coro from_this_thread() {
						_ctx_t *cur_ctx;
						cur_ctx = _tls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{2},
								{true},
								{false},
								nullptr,
								nullptr,
								nullptr,
								nullptr
							};
							_tls_ctx().set(cur_ctx);
						} else {
							++cur_ctx->use_count;
						}
						return cur_ctx;
					}

					using native_handle_t = void *;
					using id_t = void *;

					constexpr coro() : _ctx(nullptr) {}

					static constexpr size_t default_stack_size = 0;

					coro(std::function<void()> start, size_t stack_size = default_stack_size) : _ctx(
						new _ctx_t{
							{1},
							{true},
							{false},
							nullptr,
							new uint8_t[stack_size],
							std::move(start),
							nullptr
						}
					) {
						_ctx->cnt.remake(&_cont_func, _ctx, _ctx->stack, stack_size);
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
						return nullptr;
					}

					id_t id() const {
						return _ctx;
					}

					bool joinable() const {
						return _ctx->joinable;
					}

					bool join() {
						assert(_ctx);

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
						assert(_ctx);

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
						assert(_ctx->use_count);
						if (!--_ctx->use_count) {
							_del(_ctx);
						}
						_ctx = nullptr;
					}

				private:
					struct _ctx_t {
						std::atomic<size_t> use_count;
						std::atomic<bool> joinable, exited;
						exec::cont cnt;
						uint8_t *stack;
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
						_ctx_t *cur_ctx = _tls_ctx().get().to<_ctx_t *>();
						if (!cur_ctx) {
							cur_ctx = new _ctx_t{
								{1},
								{false},
								{false},
								nullptr,
								nullptr,
								nullptr,
								nullptr
							};
						}
						_tls_ctx().set(_ctx);

						// A cur_ctx->use_count ownership form cur_ctx move to _ctx->joiner.
						_ctx->joiner = cur_ctx;
						cur_ctx->cnt.push_and_pop(_ctx->cnt);

						// on back
						cur_ctx->handle_joiner();
					}

					static void _del(_ctx_t *_ctx) {
						assert(!_ctx->use_count);

						if (_ctx->stack) {
							delete _ctx->stack;
						}
						delete _ctx;
					}

					static cp::tls &_tls_ctx() {
						static cp::tls inst;
						return inst;
					}

					static void _cont_func(any_word _cur_ctx) {
						auto cur_ctx = _cur_ctx.to<_ctx_t *>();
						cur_ctx->handle_joiner();
						cur_ctx->start();
						cur_ctx->exited = true;
						if (cur_ctx->joiner && cur_ctx->joiner->joinable.exchange(false)) {
							// A cur_ctx->use_count ownership form cur_ctx move to cur_ctx->joiner->joiner.
							cur_ctx->joiner->joiner = cur_ctx;
							_tls_ctx().set(cur_ctx->joiner);
							cur_ctx->joiner->cnt.pop();
						}
						assert(--cur_ctx->use_count); // There is no way to delete cur_ctx->stack.
					}
			};
		}
	}
}

#endif
