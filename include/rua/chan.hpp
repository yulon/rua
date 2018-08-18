#ifndef _RUA_CHAN_HPP
#define _RUA_CHAN_HPP

#include "sched.hpp"
#include "macros.hpp"

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <cassert>

namespace rua {
	template <typename T>
	class chan {
		public:
			chan() : _ctx(std::make_shared<_ctx_t>()) {}

		private:
			struct _ctx_t;

		public:
			class locked_t {
				public:
					locked_t(chan<T> ch, bool try_lock = false, scheduler *sch = nullptr) {
						if (!ch._ctx) {
							return;
						}
						if (try_lock) {
							if (!ch._ctx->mtx.try_lock()) {
								_sch = nullptr;
								return;
							}
							_sch = sch ? sch : &get_scheduler();
							_ctx = std::move(ch._ctx);
						} else {
							_sch = sch ? sch : &get_scheduler();
							_ctx = std::move(ch._ctx);
							_sch->lock(_ctx->mtx);
						}
					}

					~locked_t() {
						unlock();
					}

					locked_t(const locked_t &) = delete;

					locked_t &operator=(const locked_t &) = delete;

					locked_t(locked_t &&) = default;

					locked_t &operator=(locked_t &&) = default;

					locked_t &operator<<(T value) {
						assert(_ctx);

						_ctx->buffer.emplace(std::move(value));

						return *this;
					}

					locked_t &operator<<(std::queue<T> values) {
						assert(_ctx);

						while (values.size()) {
							_ctx->buffer.emplace(std::move(values.front()));
							values.pop();
						}

						return *this;
					}

					void wait_inputs() {
						assert(_ctx);

						auto ctx = _ctx;
						auto cv = _sch->make_cond_var();
						_sch->cond_wait(cv, ctx->mtx, [ctx, cv]()->bool {
							if (ctx->buffer.size()) {
								return true;
							}
							ctx->reqs.emplace(cv);
							return false;
						});
					}

					template <typename R>
					locked_t &operator>>(R &receiver) {
						RUA_STATIC_ASSERT((std::is_convertible<T, R>::value));

						wait_inputs();

						receiver = std::move(_ctx->buffer.front());
						_ctx->buffer.pop();

						return *this;
					}

					T get() {
						T r;
						*this >> r;
						return r;
					}

					std::queue<T> get_all() {
						wait_inputs();
						return std::move(_ctx->buffer);
					}

					T try_get() {
						T r;
						if (_ctx->buffer.size()) {
							r = std::move(_ctx->buffer.front());
							_ctx->buffer.pop();
						}
						return r;
					}

					std::queue<T> try_get_all() {
						return std::move(_ctx->buffer);
					}

					void unlock() {
						if (!_ctx) {
							return;
						}
						if (_ctx->buffer.size() && _ctx->reqs.size()) {
							auto cv = _ctx->reqs.front();
							_ctx->reqs.pop();
							_ctx->mtx.unlock();
							cv->notify();
							return;
						}
						_ctx->mtx.unlock();
					}

					operator bool() const {
						return _ctx.get();
					}

				private:
					std::shared_ptr<_ctx_t> _ctx;
					scheduler *_sch;
			};

			typename chan<T>::locked_t lock(scheduler *sch = nullptr) const {
				return locked_t(*this, false, sch);
			}

			typename chan<T>::locked_t try_lock(scheduler *sch = nullptr) const {
				return locked_t(*this, true, sch);
			}

			template <typename V>
			typename chan<T>::locked_t operator<<(V &&value) {
				auto ioer = lock();
				ioer << std::forward<V>(value);
				return ioer;
			}

			typename chan<T>::locked_t wait_inputs(scheduler *sch = nullptr) {
				auto ioer = lock(sch);
				ioer.wait_inputs();
				return ioer;
			}

			template <typename R>
			typename chan<T>::locked_t operator>>(R &receiver) {
				auto ioer = lock();
				ioer >> receiver;
				return ioer;
			}

			T get(scheduler *sch = nullptr) {
				return lock(sch).get();
			}

			std::queue<T> get_all(scheduler *sch = nullptr) {
				return lock(sch).get_all();
			}

			T try_get(scheduler *sch = nullptr) {
				auto ioer = try_lock(sch);
				if (ioer) {
					return ioer.try_get_all();
				}
				return T();
			}

			std::queue<T> try_get_all(scheduler *sch = nullptr) {
				auto ioer = try_lock(sch);
				if (ioer) {
					return ioer.try_get_all();
				}
				return std::queue<T>();
			}

		private:
			struct _ctx_t {
				std::mutex mtx;
				std::queue<T> buffer;
				std::queue<std::shared_ptr<scheduler::cond_var>> reqs;
			};

			std::shared_ptr<_ctx_t> _ctx;
	};
}

#endif
