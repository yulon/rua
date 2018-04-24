#ifndef _RUA_CP_CORO_UNI_HPP
#define _RUA_CP_CORO_UNI_HPP

#include "../../exec/cont.hpp"

#include <functional>
#include <atomic>
#include <memory>
#include <string>
#include <cassert>

namespace rua {
	namespace cp {
		class coro_joiner {
			public:
				using native_handle_t = exec::cont;

				constexpr coro_joiner() : _cnt(), _joinable(false) {}

				constexpr coro_joiner(native_handle_t cnt) : _cnt(cnt), _joinable(true) {}

				~coro_joiner() {
					reset();
				}

				coro_joiner(const coro_joiner &) = default;

				coro_joiner &operator=(const coro_joiner &) = default;

				coro_joiner(coro_joiner &&src) : _cnt(src._cnt), _joinable(src._joinable) {
					if (src) {
						src.reset();
					}
				}

				coro_joiner &operator=(coro_joiner &&src) {
					if (src) {
						_cnt = src._cnt;
						_joinable = true;
						src.reset();
					} else {
						reset();
					}
					return *this;
				}

				const native_handle_t &native_handle() const {
					return _cnt;
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
					_cnt.pop();
				}

				void join(coro_joiner &get_cur) {
					assert(_joinable);

					_joinable = false;
					get_cur._joinable = true;
					get_cur._cnt.push_and_pop(_cnt);
				}

				template <typename... A>
				void operator()(A&&... a) const {
					return join(std::forward<A>(a)...);
				}

				void reset() {
					_joinable = false;
				}

			protected:
				native_handle_t _cnt;
				bool _joinable;
		};

		class coro : public coro_joiner {
			public:
				using joiner_t = coro_joiner;

				static constexpr size_t default_stack_size = 8 * 1024;

				constexpr coro() : coro_joiner(), _res(nullptr) {}

				coro(std::function<void()> func, size_t stack_size = default_stack_size) : _res(
					new _res_t{ std::unique_ptr<uint8_t[]>(new uint8_t[stack_size]), std::move(func) }
				) {
					_cnt.remake(&_cont_func, _res.get(), _res->stk.get(), stack_size);
					_joinable = true;
				}

				~coro() {
					reset();
				}

				coro(const coro &) = delete;

				coro &operator=(const coro &) = delete;

				coro(coro &&src) = default;

				coro &operator=(coro &&src) = default;

				operator bool() const {
					return _res.get();
				}

				void reset() {
					if (!*this) {
						return;
					}
					coro_joiner::reset();
					_res.reset();
				}

			protected:
				struct _res_t {
					std::unique_ptr<uint8_t[]> stk;
					std::function<void()> func;
				};

				std::unique_ptr<_res_t> _res;

				static void _cont_func(any_word res) {
					res.to<_res_t *>()->func();
					exit(0);
				}
		};
	}
}

#endif
